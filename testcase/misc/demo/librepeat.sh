repeat_clean()
{
	rm -rf *.log
	st_s_run pkill sctp_test
	st_log DBG "kill sctp_test on server"
}
