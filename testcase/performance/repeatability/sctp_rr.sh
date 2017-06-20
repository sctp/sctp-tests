Name="sctp repeatability - sctp-rr" # Case Name, demand
Topo="netns_cs" # Optional
source ./librpt.sh

do_setup()
{
	rpt_env_init
	st_s_run netserver
}

do_clean()
{
	st_s_run pkill netserver
}

do_test()
{
	LOG_DIR="rpt-sctp-rr"

	st_log INFO "Test case 2: RR SCTP"
	LOG_PREFIX="rr-sctp"
	NETEM=""
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_RR \
			-H ${st_s_ip4[0]} -L ${st_c_ip4[0]} \
			-- -m 1024"
	rpt_test

	st_log INFO "Test case 4: RR SCTP IPv6"
	LOG_PREFIX="rr-sctp-ipv6"
	NETEM=""
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_RR -6 \
			-H ${st_s_ip6[0]} -L ${st_c_ip6[0]} \
			-- -6 -m 1024"
	rpt_test

	st_log INFO "Test case 2: RR 20ms SCTP"
	LOG_PREFIX="rr-20ms-sctp"
	NETEM="delay 20ms"
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_RR \
			-H ${st_s_ip4[0]} -L ${st_c_ip4[0]} \
			-- -m 1024"
	rpt_test

	st_log INFO "Test case 4: RR 20ms SCTP IPv6"
	LOG_PREFIX="rr-20ms-sctp-ipv6"
	NETEM="delay 20ms"
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_RR -6 \
			-H ${st_s_ip6[0]} -L ${st_c_ip6[0]} \
			-- -6 -m 1024"
	rpt_test
}
