/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_P_H
#define BGP_P_H

#include "nlri.h"
#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"

namespace ns3 {

class BGPPeer : public Object {

	public:
	static TypeId GetTypeId (void);
	Ipv4Address* getAddress();
	uint32_t getAsn();
	BGPPeer();

	private:
	uint32_t m_peer_as;
	Ipv4Address m_peer_addr;
};

}

#endif /* BGP_P_H */

