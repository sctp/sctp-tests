Name="extoverflow"

do_setup()
{
	modprobe sctp

	sysctl -w net.sctp.auth_enable=1
	sysctl -w net.sctp.addip_enable=1
	sysctl -w net.sctp.prsctp_enable=1
	sysctl -w net.sctp.reconf_enable=1

	sctp_test -H 127.0.0.1 -P 5001 -l > /dev/null 2>&1 &
	sctp_test_pid=$!
}

do_clean()
{
	sysctl -w net.sctp.auth_enable=0
	sysctl -w net.sctp.addip_enable=0
	sysctl -w net.sctp.prsctp_enable=0
	sysctl -w net.sctp.reconf_enable=0

	kill $sctp_test_pid
	rm -rf *.log
}

do_test()
{
	local logf="$(st_o).log"

	sctp_test -H 127.0.0.1 -P 1234 -h 127.0.0.1 -p 5001 -s -c 5 -x 1000 -T \
								> $logf 2>&1

	st_log INFO "- PASS - No Panic"
}
