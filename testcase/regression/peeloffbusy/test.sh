Name="peeloffbusy"

do_setup()
{
	sctp_darn -H 127.0.0.1 -P 5001 -l > /dev/null 2>&1 &
	gcc client.c -o client -lsctp -lpthread
}

do_clean()
{
	local logf="$(st_o).log"

	pkill sctp_darn
	rm client $logf -rf
}

do_test()
{
	local logf="$(st_o).log"
	local res=PASS

	./client 127.0.0.1 5001 > $logf 2>&1 || res=FAIL

	st_log INFO "- $res -"
}
