Name="procdumps"

pd_send()
{
	local logf="$(st_o)_send.log"

	for port in {4000..4500}; do
		for cases in {0..6}; do
			sctp_test -H 127.0.0.1 -P ${port}${cases} \
				  -h 127.0.0.1 -p 1234 -s -c $cases \
					2>&1 >> $logf &
		done
		sleep 0.5
	done
}

pd_dump()
{
	for i in `seq 20000`; do
		cat /proc/net/sctp/assocs > /dev/null
		cat /proc/net/sctp/eps > /dev/null
		cat /proc/net/sctp/remaddr > /dev/null
		cat /proc/net/sctp/snmp > /dev/null
	done
}

do_setup()
{
	sctp_test -H 127.0.0.1 -P 1234 -l 2>&1 > /dev/null &
}

do_clean()
{
	pkill sctp_test
	rm -rf *.log
}

do_test()
{
	local sndid

	st_log DBG "start to create/close assoc"
	pd_send &
	sndid=$!

	st_log DBG "dump sctp proc in parallel"
	pd_dump
	st_log DBG "wait send process"
	wait $sndid

	st_log INFO "- PASS - No Panic"
}
