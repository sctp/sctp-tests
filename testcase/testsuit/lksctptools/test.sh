Name="lksctptools"

do_setup()
{
	git clone https://github.com/sctp/lksctp-tools
	pushd lksctp-tools
	./bootstrap && ./configure && make
	popd
}

do_clean()
{
	rm -rf lksctp-tools
}

do_test()
{
	local res=PASS

	pushd lksctp-tools/src/func_tests
	make v4test || res=FAIL
	st_log INFO "- $res - v4test "

	res=PASS
	make v6test || res=FAIL
	st_log INFO "- $res - v6test"
	popd
}
