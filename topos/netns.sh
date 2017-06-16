st_netns_is_host()
{
	[ "$ns" = "h" ] && return 0
}

st_netns_run_create()
{
	local ns=$1

	eval "  st_${ns}_run()
		{
			cat <<- EOF | ip netns exec $(st_o)-$ns bash
			\$@
			EOF
		}"
}

st_netns_veth_assign()
{
	local orig=$1; local ns=$2; local veth=$3;

	if st_netns_is_host $ns; then
		ip link set $orig name $veth
		ip link set $veth up
	else
		ip link set $orig name $veth netns $(st_o)-$ns
		st_${ns}_run "ip link set $veth up"
	fi
}

# exported
st_netns_ns_create()
{
	local netns=$@

	for ns in $netns; do
		st_netns_run_create $ns
		ip netns add $(st_o)-$ns
		st_${ns}_run "ip link set lo up"
	done
}

# exported
st_netns_veth_create()
{
	local ns1=$1; local veth1=($2)
	local ns2=$3; local veth2=($4)

	for i in "${!veth1[@]}" ; do
		ip link add veth0 type veth peer name veth1
		st_netns_veth_assign veth0 $ns1 ${veth1[$i]}
		st_netns_veth_assign veth1 $ns2 ${veth2[$i]}
	done
}

# exported
st_netns_addr_assign()
{
	local netns=$1;    local veths=($2)
	local prefix=($4); local addrs=($3)

	for i in "${!veths[@]}" ; do
		st_${netns}_run \
		ip a a ${addrs[$i]}/${prefix[$i]} dev ${veths[$i]}
	done
}

# exported
st_netns_ns_destroy()
{
	local netns=$@

	for ns in $netns
	do
		ip netns del $(st_o)-$ns
		unset st_${ns}_run
	done
}

# exported
st_netns_team_create()
{
	local netns=$1;  local team=$2
	local slaves=$3; local team_opt="$4"

	st_${netns}_run teamd -d -t $team -c $team_opt
	st_${netns}_run ip link set $team up

	for slave in $slaves ; do
		st_${netns}_run ip link set $slave down
		st_${netns}_run teamdctl $team port add $slave
		st_${netns}_run ip link set $slave up
	done
}

# exported
st_netns_team_destroy()
{
	local netns=$1; local teams=$2

	for team in $teams ; do
		st_${netns}_run teamd -t $team -k
	done
}

# exported
st_netns_ipsec_getopt()
{
	local opts="$2"; local name="$1"
	local last=""

	for opt in $opts; do
		[ "$name" = "$last" ] && {
			echo $opt
			break
		}
		last=$opt
	done
}

# exported
st_netns_ipsec_create()
{
	local netns=$1;   local locals=($2)
	local peers=($3); local ipsec="$4"
	local localt=($5);local peert=($6)
	local dir="$7"

	local mode=$(st_netns_ipsec_getopt "mode" "$ipsec")
	local proto=$(st_netns_ipsec_getopt "proto" "$ipsec")
	local bdir="in"; local tnl=""

	[ $dir = $bdir ] && bdir="out"
	IFS='|'; ipsec=($ipsec); unset IFS
	for i in "${!locals[@]}" ; do
		[ $mode = "tunnel" ] && tnl="src ${localt[$i]} dst ${peert[$i]}"
		st_${netns}_run ip xfrm state add src ${locals[$i]} dst ${peers[$i]} ${ipsec[0]}
		st_${netns}_run ip xfrm policy add dir $dir src ${locals[$i]} dst ${peers[$i]} \
			proto any tmpl $tnl proto $proto mode $mode level required

		[ $mode = "tunnel" ] && tnl="src ${peert[$i]} dst ${localt[$i]}"
		st_${netns}_run ip xfrm state add src ${peers[$i]} dst ${locals[$i]} ${ipsec[1]}
		st_${netns}_run ip xfrm policy add dir $bdir src ${peers[$i]} dst ${locals[$i]} \
			proto any tmpl $tnl proto $proto mode $mode level required
	done
}

# exported
st_netns_ipsec_destroy()
{
	local netns=$1

	st_${netns}_run ip xfrm state flush
	st_${netns}_run ip xfrm policy flush
	st_${netns}_run ip -6 xfrm state flush
	st_${netns}_run ip -6 xfrm policy flush
}

# exported
st_netns_route_fwd()
{
	local netns=$@

	for ns in $netns
	do
		st_${ns}_run sysctl -w net.ipv4.conf.all.forwarding=1
		st_${ns}_run sysctl -w net.ipv6.conf.all.forwarding=1
	done
}

# exported
st_netns_route_add()
{
	local netns=$1;   local addrs=($2)
	local veths=($4); local gates=($3)

	for i in "${!addrs[@]}" ; do
		st_${netns}_run \
		ip r a ${addrs[$i]} via ${gates[$i]} dev ${veths[$i]}
	done
}
