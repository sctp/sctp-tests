Name="ndatasched"
Topo="netns_cs"

do_setup()
{
	modprobe sctp

	st_c_run sysctl -w net.sctp.reconf_enable=1
	st_s_run sysctl -w net.sctp.reconf_enable=1

	gcc strreset.c -o strreset -lsctp
	sysctl -w kernel.panic_on_warn=1
}

do_clean()
{
	st_c_run sysctl -w net.sctp.reconf_enable=0
	st_s_run sysctl -w net.sctp.reconf_enable=0

	rm strreset *.log -rf
}

do_test()
{
	local logfc="$(st_o)_c.log"
	local logfs="$(st_o)_s.log"

	for e in 0; do
		st_c_run sysctl -w net.sctp.intl_enable=$e
		st_s_run sysctl -w net.sctp.intl_enable=$e
		for c in 0 1 2 3 4 5 6; do
			local res=PASS

			st_c_run ./strreset ${st_s_ip4[0]} 8888 $c -s \
							>> $logfc 2>&1 &
			sctp_ndata_pid=$!
			st_s_run ./strreset ${st_s_ip4[0]} 8888 $c -l \
							>> $logfs || res=FAIL
			wait $sctp_ndata_pid

			st_log INFO "- $res - case $c"
		done
	done
	st_c_run sysctl -w net.sctp.intl_enable=0
	st_s_run sysctl -w net.sctp.intl_enable=0
}
