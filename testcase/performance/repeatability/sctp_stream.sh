Name="sctp repeatability - sctp-stream" # Case Name, demand
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
	LOG_DIR="rpt-sctp-stream"

	st_log INFO "Test case 6: STREAM SCTP"
	LOG_PREFIX="stream-sctp"
	NETEM=""
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_STREAM \
			-H ${st_s_ip4[0]} -L ${st_c_ip4[0]} \
			-- -m 1024"
	rpt_test

	st_log INFO "Test case 8: STREAM SCTP IPv6"
	LOG_PREFIX="stream-sctp-ipv6"
	NETEM=""
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_STREAM -6 \
			-H ${st_s_ip6[0]} -L ${st_c_ip6[0]} \
			-- -6 -m 1024"
	rpt_test

	st_log INFO "Test case 6: STREAM 20ms SCTP"
	LOG_PREFIX="stream-20ms-sctp"
	NETEM="delay 20ms"
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_STREAM \
			-H ${st_s_ip4[0]} -L ${st_c_ip4[0]} \
			-- -m 1024"
	rpt_test

	st_log INFO "Test case 8: STREAM 20ms SCTP IPv6"
	LOG_PREFIX="stream-20ms-sctp-ipv6"
	NETEM="delay 20ms"
	NETPERF_PARAMS="$NETPERF_BASE -t SCTP_STREAM -6 \
			-H ${st_s_ip6[0]} -L ${st_c_ip6[0]} \
			-- -6 -m 1024"
	rpt_test
}
