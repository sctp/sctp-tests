source "$ROOTDIR/topos/netns.sh"

st_topo_setup()
{
	local def_opt="ttl 64"
	local tnl_opt=${Topo_opt:-"$st_sit_opt"}

	tnl_opt=${tnl_opt:-"$def_opt"}

	st_c_ethx=(eth1 eth2)
	st_s_ethx=(eth1 eth2)
	st_c_eth=(sitc1 sitc2)
	st_s_eth=(sits1 sits2)

	st_c_ip6=(2000::1 1000::1)
	st_s_ip6=(2000::2 1000::2)
	st_c_ip4=(192.0.0.1 172.0.0.1)
	st_s_ip4=(192.0.0.2 172.0.0.2)

	st_netns_ns_create c s
	st_netns_veth_create c "${st_c_ethx[*]}" s "${st_s_ethx[*]}"

	st_netns_addr_assign c "${st_c_ethx[*]}" "${st_c_ip4[*]}" "24 16"
	st_netns_addr_assign s "${st_s_ethx[*]}" "${st_s_ip4[*]}" "24 16"

	st_netns_tnl_create c "${st_c_eth[*]}" "${st_c_ip4[*]}" \
			      "${st_s_ip4[*]}" "sit" "$tnl_opt"
	st_netns_tnl_create s "${st_s_eth[*]}" "${st_s_ip4[*]}" \
			      "${st_c_ip4[*]}" "sit" "$tnl_opt"

	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip6[*]}" "64 64"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip6[*]}" "64 64"

}

st_topo_clean()
{
	st_netns_dev_destroy c "${st_c_eth[*]}"
	st_netns_dev_destroy s "${st_s_eth[*]}"
	st_netns_ns_destroy c s
}
