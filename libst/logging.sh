declare -A st_logs=([DBG]=1 [INFO]=2  [WARN]=3  [ERR]=4)

st_log_check()
{
	local L=$LogLevel l=$1

	[ "${st_logs[$l]}" \< "${st_logs[$L]}" ] && {
		return 1
	}

	return 0
}

st_log_valid()
{
	[ -n "$LogLevel" ] || return 1
}

st_log_format()
{
	local level=$1; shift
	local context="$*"

	printf "[%s] [%4s]: %s" \
		"$(date +%s)" "$level" "$context"
}

st_log_print()
{
	local st_log=${LogFile:-"$ROOTDIR/sctp-tests.log"}
	local output=${Output:-"/tmp/output.log"}
	local format=$(st_log_format $@)

	echo $format >&3
	echo $format >> $st_log
	echo $format >> $output
}

st_log_echo()
{
	local context="$*"

	echo $context
}

st_log()
{
	local level=INFO context=""

	[ $# -lt 2 ] || {
		level=$1; shift
	}
	context="$*"

	st_log_valid || {
		st_log_echo $context
		return 0
	}

	st_log_check $level && {
		st_log_print $level $context
	}

	return 0
}
