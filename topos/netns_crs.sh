source "$ROOTDIR/topos/netns.sh"

st_topo_setup()
{
	st_c_eth=(eth1 eth2)
	st_s_eth=(eth1 eth2)

	st_c_ip6=(2000::1 1000::1)
	st_s_ip6=(2001::2 1001::2)
	st_c_ip4=(192.0.0.1 172.0.0.1)
	st_s_ip4=(192.0.1.2 172.0.1.2)

	st_r_eth=(ceth1 ceth2 seth1 seth2)
	st_r_ip6=(2000::2 1000::2 2001::1 1001::1)
	st_r_ip4=(192.0.0.2 172.0.0.2 192.0.1.1 172.0.1.1)

	st_netns_ns_create c s r
	st_netns_veth_create c "${st_c_eth[*]}" r "${st_r_eth[*]:0:2}"
	st_netns_veth_create s "${st_s_eth[*]}" r "${st_r_eth[*]:2:2}"

	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip4[*]}" "24 24"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip4[*]}" "24 24"
	st_netns_addr_assign r "${st_r_eth[*]}" "${st_r_ip4[*]}" "24 24 24 24"
	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip6[*]}" "64 64"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip6[*]}" "64 64"
	st_netns_addr_assign r "${st_r_eth[*]}" "${st_r_ip6[*]}" "64 64 64 64"

	st_netns_route_fwd r
	st_netns_route_add c "${st_s_ip4[*]}" "${st_r_ip4[*]:0:2}" "${st_c_eth[*]}"
	st_netns_route_add s "${st_c_ip4[*]}" "${st_r_ip4[*]:2:2}" "${st_s_eth[*]}"
	st_netns_route_add c "${st_s_ip6[*]}" "${st_r_ip6[*]:0:2}" "${st_c_eth[*]}"
	st_netns_route_add s "${st_c_ip6[*]}" "${st_r_ip6[*]:2:2}" "${st_s_eth[*]}"
}

st_topo_clean()
{
	st_netns_ns_destroy c s r
}
