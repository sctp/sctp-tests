Name="ndatasched"
Topo="netns_cs"

do_setup()
{
	modprobe sctp

	st_c_run sysctl -w net.sctp.reconf_enable=1
	st_s_run sysctl -w net.sctp.reconf_enable=1

	gcc sctp_ndata.c -o sctp_ndata -lsctp
	sysctl -w kernel.panic_on_warn=1
}

do_clean()
{
	st_c_run sysctl -w net.sctp.reconf_enable=0
	st_s_run sysctl -w net.sctp.reconf_enable=0

	rm sctp_ndata *.log -rf
}

do_test()
{
	local logfc="$(st_o)_c.log"
	local logfs="$(st_o)_s.log"

	for e in 0 1;do
		st_c_run sysctl -w net.sctp.intl_enable=$e
		st_s_run sysctl -w net.sctp.intl_enable=$e
		for c in 0 1 5 6 7 11 12 13 17 18; do
			local res=PASS

			st_c_run ./sctp_ndata ${st_s_ip4[0]} 8888 $c -s \
							>> $logfc 2>&1 &
			sctp_ndata_pid=$!
			st_s_run ./sctp_ndata ${st_s_ip4[0]} 8888 $c -l \
							>> $logfs || res=FAIL
			wait $sctp_ndata_pid

			st_log INFO "- $res - case $c"
		done
		for c in 2 3 4 8 9 10 14 15 16 19 20; do
			local res="xx mb/s"

			st_c_run ./sctp_ndata ${st_s_ip4[0]} 8888 $c -s \
							>> $logfc 2>&1 &
			sctp_ndata_pid=$!
			res=`st_s_run ./sctp_ndata ${st_s_ip4[0]} 8888 $c -l`
			wait $sctp_ndata_pid

			st_log INFO "- $res - case $c"
		done
	done
	st_c_run sysctl -w net.sctp.intl_enable=0
	st_s_run sysctl -w net.sctp.intl_enable=0
}
