/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_P_H
#define BGP_P_H

#include "bgp-filter.h"
#include "bgp-route.h"
#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"


namespace ns3 {

class BGPPeer : public Object {

	public:
	static TypeId GetTypeId (void);
	Ipv4Address getAddress();
	void setAddress(Ipv4Address addr);
	uint32_t getAsn();
	void setAsn(uint32_t asn);
	BGPPeer();
	uint32_t m_peer_as;
	uint32_t m_peer_dev_id;
	Ipv4Address m_peer_addr;
	BGPFilterRules *out_filter;
	BGPFilterRules *in_filter;

	bool passive;
};

}

#endif /* BGP_P_H */

