Name="sctphashtable - on host"
Topo="phost_cs"

sht_ip4="172.16"
sht_ip6="2020"
sht_port=6400
sht_cnt=50

sht_ip4_setup()
{
	st_c_run ip addr flush ${st_c_eth[0]}
	st_log DBG "add $sht_cnt x $sht_cnt ip4 addrs $sht_ip4.x.y on client"
	st_c_run "
	for i in \$(seq $sht_cnt);do
		for j in \$(seq $sht_cnt);do
			ip a a $sht_ip4.\$i.\$j/16 dev ${st_c_eth[0]}
		done
	done"
	st_c_run ip a a $sht_ip4.253.253/16 dev ${st_c_eth[0]}

	st_log DBG "add ip4 addrs $sht_ip4.254.254 on server"
	st_s_run ip addr flush ${st_s_eth[0]}
	st_s_run ip a a $sht_ip4.254.254/16 dev ${st_s_eth[0]}
}

sht_ip6_setup()
{
	st_c_run ip -6 addr flush ${st_c_eth[0]}
	st_log DBG "add $sht_cnt x $sht_cnt ip6 addrs $sht_ip6::x:y on client"
	st_c_run "
	for i in \$(seq $sht_cnt);do
		for j in \$(seq $sht_cnt);do
			ip -6 a a $sht_ip6::\$i:\$j/64 dev ${st_c_eth[0]}
		done
	done"
	st_c_run ip -6 a a $sht_ip6::253:253/64 dev ${st_c_eth[0]}

	st_log DBG "add ip6 addrs $sht_ip6::254:254 on server"
	st_s_run ip -6 addr flush ${st_s_eth[0]}
	st_s_run ip -6 a a $sht_ip6::254:254/64 dev ${st_s_eth[0]}
}

sht_sysctl_setup()
{
	st_c_run modprobe sctp
	st_c_run sysctl -w net.sctp.sndbuf_policy=1
	st_c_run sysctl -w net.sctp.pf_enable=0
	st_c_run sysctl -w net.sctp.hb_interval=10000000
	st_c_run sysctl -w net.ipv4.neigh.default.gc_thresh3=40960
	st_c_run sysctl -w net.ipv6.neigh.default.gc_thresh3=40960

	st_s_run modprobe sctp
	st_s_run sysctl -w net.sctp.sndbuf_policy=1
	st_s_run sysctl -w net.sctp.pf_enable=0
	st_s_run sysctl -w net.sctp.hb_interval=10000000
	st_s_run sysctl -w net.ipv4.neigh.default.gc_thresh3=40960
	st_s_run sysctl -w net.ipv6.neigh.default.gc_thresh3=40960
}

do_clean()
{
	st_c_run sysctl -w net.sctp.hb_interval=30000
	st_s_run sysctl -w net.sctp.hb_interval=30000

	st_c_run pkill sctp_darn
	st_s_run pkill sctp_darn

	st_c_run rm -rf client client.c
	st_s_run rm -rf client client.c

	st_log DBG "flush ip4 ip6 addrs"
	st_c_run ip addr flush ${st_c_eth[0]}
	st_c_run ip -6 addr flush ${st_c_eth[0]}
}

do_setup()
{
	sht_sysctl_setup

	sht_ip4_setup
	st_s_run "sctp_darn -H $sht_ip4.254.254 -P $sht_port -l > /dev/null 2>&1 &"

	sht_ip6_setup ; sleep 5
	st_s_run "sctp_darn -H $sht_ip6::254:254 -P $sht_port -l > /dev/null 2>&1 &"
}

do_test()
{
	local result
	local output

	st_c_put client.c
	st_c_run gcc -o client client.c -lsctp

	st_log INFO "client4 start"
	result=PASS
	output=$(st_c_run "ulimit -n 40960; timeout 1000 ./client \
		$sht_ip4.254.254 $sht_port 1234 $sht_cnt") || result=FAIL
	st_log INFO "- $result - $output"

	st_log INFO "client6 start"
	result=PASS
	output=$(st_c_run "ulimit -n 40960; timeout 1000 ./client \
		$sht_ip6::254:254 $sht_port 1234 $sht_cnt") || result=FAIL
	st_log INFO "- $result - $output"
}
