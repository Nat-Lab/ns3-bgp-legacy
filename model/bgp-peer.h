/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_P_H
#define BGP_P_H

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

	private:
	uint32_t m_peer_as;
	Ipv4Address m_peer_addr;
};

}

#endif /* BGP_P_H */

