source "$ROOTDIR/topos/phost.sh"

st_topo_setup()
{
	st_c_eth=($Host1_eth1)
	st_s_eth=($Host2_eth1)

	st_c_ip6=(2000::1)
	st_s_ip6=(2000::2)
	st_c_ip4=(192.0.0.1)
	st_s_ip4=(192.0.0.2)

	st_phost_ph_create "c s" "$Host1 $Host2"
	st_phost_addr_assign c "${st_c_eth[*]}" "${st_c_ip4[*]}" "24"
	st_phost_addr_assign s "${st_s_eth[*]}" "${st_s_ip4[*]}" "24"
	st_phost_addr_assign c "${st_c_eth[*]}" "${st_c_ip6[*]}" "64"
	st_phost_addr_assign s "${st_s_eth[*]}" "${st_s_ip6[*]}" "64"
}

st_topo_clean()
{
	st_phost_addr_flush c "${st_c_eth[*]}"
	st_phost_addr_flush s "${st_s_eth[*]}"
	st_phost_ph_destroy c s
}
