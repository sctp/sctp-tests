Name="sctpdiag"

sctpdiag_send()
{
	local logf="$(st_o)_send.log"

	for i in `seq 30000`; do
		sctp_test -H 127.0.0.1 -h 127.0.0.1 -p 1234 -s \
						2>&1 >> $logf
	done
}

sctpdiag_ss()
{
	local logf="$(st_o)_ss.log"

	for i in `seq 100000`; do
		ss --sctp 2>&1 >> $logf
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
	sctpdiag_send &
	sndid=$!

	st_log DBG "ss show sctp in parallel"
	sctpdiag_ss
	st_log DBG "wait send process"
	wait $sndid

	st_log INFO "- PASS - No Panic"
}
