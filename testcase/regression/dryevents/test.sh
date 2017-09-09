Name="dryevents"

do_setup()
{
	gcc sctp-dry-event.c -o sctp-dry-event -lsctp
	./sctp-dry-event server dry > /dev/null 2>&1 &
	sctp_dry_pid=$!
}

do_clean()
{
	local logf="$(st_o).log"

	kill $sctp_dry_pid
	rm sctp-dry-event $logf -rf
}

do_test()
{
	local logf="$(st_o).log"
	local res=PASS

	for i in {1..300}; do
		./sctp-dry-event client > $logf 2>&1 || res=FAIL
	done

	st_log INFO "- $res -"
}
