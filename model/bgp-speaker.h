/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_S_H
#define BGP_S_H

#include "bgp-peer.h"
#include "nlri.h"
#include "ns3/application.h"
#include "ns3/uinteger.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"
#include "ns3/vector.h"

namespace ns3 {

class BGPSpeaker : public Application {

	public:
	static TypeId GetTypeId (void);
	BGPSpeaker();

	private:
	std::vector<Ptr<BGPPeer>> m_peers;
	std::vector<Ptr<NLRI>> m_nlri;
	uint32_t m_asn;
};

}

#endif /* BGP_S_H */

