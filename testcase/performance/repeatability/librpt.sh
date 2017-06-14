NETPERF_BASE="-cC -l 10"
ITERS=3

rpt_env_netperf()
{
        local url=ftp://ftp.netperf.org/netperf/
        local name=netperf-2.7.0
	local tar=.tar.gz

	[ -a /usr/bin/netperf ] && return 0

        st_env_pushd /tmp

        wget $url$name$tar; tar zxf $name$tar; cd $name
        ./configure --enable-sctp --bindir=/usr/bin
        make && make install

        st_log DBG "netperf setup done"

        st_env_popd
}

rpt_env_datamash()
{
        local url=http://ftp.gnu.org/gnu/datamash/
        local name=datamash-1.1.0
	local tar=.tar.gz

	[ -a /usr/bin/datamash ] && return 0

        st_env_pushd /tmp

        wget $url$name$tar; tar zxf $name$tar; cd $name
        ./configure --prefix /usr
        make && make install

        st_log DBG "rpt_datamash setup done"

        st_env_popd
}

rpt_env_init()
{
	rpt_env_netperf
	rpt_env_datamash
}

rpt_snmp_log()
{
	local log=$1 i=$2 m=$3
	st_s_run cat /proc/net/sctp/snmp > $(st_o).${log}.${i}.snmp.server.a${m}
	st_c_run cat /proc/net/sctp/snmp > $(st_o).${log}.${i}.snmp.client.a${m}
}

rpt_dmsg_log()
{
        local log=$1 i=$2
        dmesg -dc > $(st_o).${log}.${i}.dmesg
}

rpt_summ_log()
{
	local log=$1 i=$2 param=$3
	local rate=""
	local vars=""

	if [[ $NETPERF_PARAMS =~ "_STREAM" ]]; then
        	rate=($(tail -n1 $(st_o).${log}.${i}.netperf))
        	rate=${rate[4]}
	else
        	rate=($(tail -n2 $(st_o).${log}.${i}.netperf | head -n1))
        	rate=${rate[5]}
	fi

	for var in $VARS; do
        	vars="$vars $(rpt_snmp_diff client $i $var $log)"
	done
	printf "$FMT" $rate $vars >> $(st_o).${log}.summary

	vars=""
	for var in $VARS; do
        	vars="$vars $(rpt_snmp_diff server $i $var $log)"
	done
	printf "$FMT" "server" $vars >> $(st_o).${log}.summary

	RATE=$rate
}

rpt_snmp_var()
{
        local file=$1
        local var=$2

        sed -n "s/^Sctp$var\\s\\+//p" $file
}

rpt_snmp_diff()
{
        local dir=$1 i=$2 var=$3 log=$4

        local before=$(rpt_snmp_var $(st_o).${log}.${i}.snmp.${dir}.abefore $var)
        local aafter=$(rpt_snmp_var $(st_o).${log}.${i}.snmp.${dir}.aafter $var)

        echo $((aafter-before))
}

rpt_rs_log()
{
	local m=$1 log=$2

	[ $m = "after" ] && {
		RS=$(sed '1d;s/^\s\+//;/server/d' < $(st_o).${log}.summary |
			LANG=C datamash -W --narm min 1 max 1 mean 1 median 1 sstdev 1 |
			awk '{print "min:"$1"  max:"$2"  mean:"$3"  median:"$4"  sstdev:"$5}')
		return 0
	}

        VARS="T3Retransmits \
                FastRetransmits \
                InPktSoftirq \
                InPktBacklog \
                InPktDiscards \
                InDataChunkDiscards"
        FMT="%8s  "

        for var in $VARS; do
                FMT="$FMT %$(echo $var | wc -c)s"
        done
        FMT="$FMT\n"

        printf "$FMT" "Rate" $VARS > $(st_o).${log}.summary
	return 0
}

rpt_perf_run()
{
	local log=$1 i=$2 param=$3
	st_c_run netperf $param >& $(st_o).${log}.${i}.netperf
}

rpt_tc_netem()
{
	local m=$1 netem=$2

	[ -z "$netem" ] && return 0

	[ $m = "after" ] && {
		st_c_run tc qdisc delete dev ${st_c_eth[0]} root
		return 0
	}

	st_c_run tc qdisc add dev ${st_c_eth[0]} root netem $netem
	return 0
}

rpt_test()
{
	st_log DBG "netperf $NETPERF_PARAMS"
	[ -d $LOG_DIR ] || mkdir $LOG_DIR

	pushd $LOG_DIR

	rpt_tc_netem before "$NETEM"
	rpt_rs_log before $LOG_PREFIX
	for i in $(seq $ITERS); do
		rpt_snmp_log $LOG_PREFIX $i before
		rpt_perf_run $LOG_PREFIX $i "$NETPERF_PARAMS"
		rpt_snmp_log $LOG_PREFIX $i after

		rpt_dmsg_log $LOG_PREFIX $i
		rpt_summ_log $LOG_PREFIX $i "$NETPERF_PARAMS"

		st_log DBG " $i Rate: $RATE"
	done
	rpt_tc_netem after "$NETEM"
	rpt_rs_log after $LOG_PREFIX

	st_log INFO "- RS: $RS -"
	popd
}
