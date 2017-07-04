source "$ROOTDIR/topos/netns.sh"

st_topo_setup()
{
	local def_opt="3 3"
	local vlan_ids=${Topo_opt:-"$st_vlan_opt"}

	vlan_ids="${vlan_ids:-"$def_opt"}"

	st_c_eth=(eth1.3 eth2.3)
	st_s_eth=(eth1.3 eth2.3)
	st_c_ethx=(eth1 eth2)
	st_s_ethx=(eth1 eth2)

	st_c_ip6=(2000::1 1000::1)
	st_s_ip6=(2000::2 1000::2)
	st_c_ip4=(192.0.0.1 172.0.0.1)
	st_s_ip4=(192.0.0.2 172.0.0.2)

	st_netns_ns_create c s
	st_netns_veth_create c "${st_c_ethx[*]}" s "${st_s_ethx[*]}"

	st_netns_vlans_create c "${st_c_eth[*]}" "${st_c_ethx[*]}" "$vlan_ids"
	st_netns_vlans_create s "${st_s_eth[*]}" "${st_s_ethx[*]}" "$vlan_ids"

	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip4[*]}" "24 16"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip4[*]}" "24 16"
	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip6[*]}" "64 128"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip6[*]}" "64 128"
}

st_topo_clean()
{
	st_netns_vlans_destroy c "${st_c_eth[*]}"
	st_netns_vlans_destroy s "${st_s_eth[*]}"
	st_netns_ns_destroy c s
}
