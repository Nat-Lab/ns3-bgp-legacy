/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_N_H
#define BGP_N_H

#include <arpa/inet.h>

#include "libbgp.h"
#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ns3/net-device.h"

namespace ns3 {

class BGPRoute : public Object {

	public:
	static TypeId GetTypeId (void);
	LibBGP::BGPRoute* toLibBGP();
	static Ptr<BGPRoute> fromLibBGP(LibBGP::BGPRoute *route);

	Ipv4Address getPrefix();
	void setPrefix(Ipv4Address prefix);
	uint8_t getLength();
	void setLength(uint8_t length);
	std::vector<uint32_t>* getAsPath();
	void setAsPath(std::vector<uint32_t> path);
	BGPRoute();

	bool operator== (const BGPRoute& other);
	bool isSame (const BGPRoute& other);

	Ipv4Address src_peer;
	Ipv4Address next_hop;
	Ptr<NetDevice> device;
	uint8_t m_prefix_len;
	Ipv4Address m_prefix;
	std::vector<uint32_t> m_as_path; // only keep as_path attrib, make life easier.

	bool local;
};

}

#endif /* BGP_N_H */

