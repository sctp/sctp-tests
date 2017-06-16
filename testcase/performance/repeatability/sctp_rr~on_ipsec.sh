source "./sctp_rr.sh"
Name="sctp repeatability - sctp-rr on ipsec" # Case Name, demand
Topo_opt="proto ah spi 0x1000 mode transport enc blowfish ipv6readylogo3descbc1to2 auth hmac\(sha1\) ipv6readylogsha11to2 | \
	  proto ah spi 0x2000 mode transport enc blowfish ipv6readylogo3descbc2to1 auth hmac\(sha1\) ipv6readylogsha12to1"
Topo="netns_ipsec_cs" # Optional
