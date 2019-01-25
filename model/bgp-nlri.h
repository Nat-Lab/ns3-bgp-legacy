/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_N_H
#define BGP_N_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"
#include "libbgp.h"

namespace ns3 {

class NLRI : public Object {

	public:
	static TypeId GetTypeId (void);
	LibBGP::BGPRoute* toRoute();
	NLRI();

	private:
	uint8_t m_prefix_len;
	Ipv4Address m_prefix;
};

}

#endif /* BGP_N_H */

