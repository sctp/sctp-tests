Name="sctp repeat test with team" # Case Name, demand
Topo="netns_team_cs" # Optional
source ./librepeat.sh

do_setup()
{
	local log4s="$(st_o)_ip4s_team.log"
	local log6s="$(st_o)_ip6s_team.log"

	st_s_run sctp_test -H ${st_s_ip4[0]} \
		-B ${st_s_ip4[1]} -P 8004 -l > $log4s 2>&1 &
	st_s_run sctp_test -H ${st_s_ip6[0]} \
		-B ${st_s_ip6[1]} -P 8006 -l > $log6s 2>&1 &
	st_log DBG "start sctp_test on server"
}

do_clean()
{
	repeat_clean
}

do_test()
{
	local log4c="$(st_o)_ip4c_team.log"
	local log6c="$(st_o)_ip6c_team.log"

	st_log DBG "start do sctp ipv4 test"
	st_c_run sctp_test -H ${st_c_ip4[0]} -B ${st_c_ip4[1]} \
		-P 7004 -h ${st_s_ip4[0]} -p 8004 -s -c 1 > $log4c

	st_log DBG "start do sctp ipv6 test"
	st_c_run sctp_test -H ${st_c_ip6[0]} -B ${st_c_ip6[1]} \
		-P 7006 -h ${st_s_ip6[0]} -p 8006 -s -c 1 > $log6c

	st_log INFO " - PASS -"
}
