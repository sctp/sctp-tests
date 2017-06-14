st_lksctp_setup()
{
	[ -a /usr/bin/sctp_darn ] && return 0

	local url=http://ncu.dl.sourceforge.net/project/lksctp/lksctp-tools/
	local name=lksctp-tools-1.0.17; local tar=.tar.gz

	st_env_pushd /tmp

	wget $url$name$tar; tar zxf $name$tar; cd $name
	./configure --libdir=/usr/lib64 --bindir=/usr/bin
	make && make install

	st_log DBG "lksctp tools setup done"

	st_env_popd
}

st_package_setup()
{
	set -x
	st_lksctp_setup
	set +x
}
