source "$ROOTDIR/topos/ph.cfg"

# exported
st_phost_ph_create()
{
	local ph=($1); local host=($2)
	local ssho="-o UserKnownHostsFile=/dev/null"
	ssho="$ssho -o StrictHostKeyChecking=no -q"

	for i in "${!ph[@]}" ; do
		eval "  st_${ph[$i]}_run()
			{
				ssh $ssho ${host[$i]} \$@
			}"
	done
}

# exported
st_phost_addr_assign()
{
	local phost=$1;    local veths=($2)
	local prefix=($4); local addrs=($3)

	for i in "${!veths[@]}" ; do
		st_${phost}_run \
		ip a a ${addrs[$i]}/${prefix[$i]} dev ${veths[$i]}
		st_${phost}_run ip l set ${veths[$i]} up
	done
}

# exported
st_phost_addr_flush()
{
	local phost=$1
	local veths=($2)

	for i in "${!veths[@]}" ; do
		st_${phost}_run ip -4 a f dev ${veths[$i]}
		st_${phost}_run ip -6 a f dev ${veths[$i]}
		st_${phost}_run ip l set ${veths[$i]} down
	done
}

# exported
st_phost_ph_destroy()
{
	local phost=($@)

	for i in "${!phost[@]}" ; do
		unset st_${phost[$i]}_run
	done
}
