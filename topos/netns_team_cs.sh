source "$ROOTDIR/topos/netns.sh"

st_topo_setup()
{
	local def_opt='{ "runner" : { "name": "roundrobin" } }'
	local team_opt=${Topo_opt:-"$st_team_opt"}

	team_opt="'${team_opt:-"$def_opt"}'"

	st_c_eth=(teamc0 teamc1)
	st_s_eth=(teams0 teams1)
	st_c_ethx=(eth1 eth2 eth3 eth4)
	st_s_ethx=(eth1 eth2 eth3 eth4)

	st_c_ip6=(2000::1 1000::1)
	st_s_ip6=(2000::2 1000::2)
	st_c_ip4=(192.0.0.1 172.0.0.1)
	st_s_ip4=(192.0.0.2 172.0.0.2)

	st_netns_ns_create c s
	st_netns_veth_create c "${st_c_ethx[*]}" s "${st_s_ethx[*]}"

	st_netns_team_create c "${st_c_eth[0]}" "${st_c_ethx[*]:0:2}" "$team_opt"
	st_netns_team_create c "${st_c_eth[1]}" "${st_c_ethx[*]:2:2}" "$team_opt"
	st_netns_team_create s "${st_s_eth[0]}" "${st_s_ethx[*]:0:2}" "$team_opt"
	st_netns_team_create s "${st_s_eth[1]}" "${st_s_ethx[*]:2:2}" "$team_opt"

	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip4[*]}" "24 16"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip4[*]}" "24 16"
	st_netns_addr_assign c "${st_c_eth[*]}" "${st_c_ip6[*]}" "64 128"
	st_netns_addr_assign s "${st_s_eth[*]}" "${st_s_ip6[*]}" "64 128"
}

st_topo_clean()
{
	st_netns_team_destroy c "${st_c_eth[*]}"
	st_netns_team_destroy s "${st_s_eth[*]}"
	st_netns_ns_destroy c s
}
