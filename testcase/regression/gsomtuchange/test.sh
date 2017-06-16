Name="gsomtuchange"
Topo="netns_crs"

do_setup()
{
	st_s_run sctp_test -H :: -P 33331 -l -T > /dev/null 2>&1 &
}

do_clean()
{
	pkill sctp_test
	rm -rf *.log
}

do_test()
{
	local mtus="1490 1480 1470 1460 1450 1440 1430 1420 1410 1400"
	local logf="$(st_o)_send.log"
	local pids=""
	local i=0

	st_log DBG "sctp_test start"
	for mtu in $mtus; do
		st_c_run sctp_test -H ${st_c_ip4[0]} -P 3333$i \
				   -h ${st_s_ip4[0]} -p 33331 -s -c 5 \
				   -x 1000 -T 2>&1 >> $logf &
		pids="$pids $!"
		st_c_run sctp_test -H ${st_c_ip6[0]} -P 3333$i \
				   -h ${st_s_ip6[0]} -p 33331 -s -c 5 \
				   -x 1000 -T 2>&1 >> $logf &
		pids="$pids $!"
		sleep 2
		st_r_run ip link set dev ${st_r_eth[0]} mtu $mtu
		st_r_run ip link set dev ${st_r_eth[2]} mtu $mtu
		let i++
	done

	st_log DBG "wait for children $pids"
	wait $pids

	st_log INFO " - PASS - No Panic"
}
