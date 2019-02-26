#ifndef BGP_ROUTING_H
#define BGP_ROUTING_H

#include <algorithm>
#include <vector>
#include <iterator>

#include "ns3/bgp-route.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/log.h"

namespace ns3 {

const uint32_t CIDR_MASK_MAP[33] = {0x00000000,0x80000000,0xc0000000,0xe0000000,0xf0000000,0xf8000000,0xfc000000,0xfe000000,0xff000000,0xff800000,0xffc00000,0xffe00000,0xfff00000,0xfff80000,0xfffc0000,0xfffe0000,0xffff0000,0xffff8000,0xffffc000,0xffffe000,0xfffff000,0xfffff800,0xfffffc00,0xfffffe00,0xffffff00,0xffffff80,0xffffffc0,0xffffffe0,0xfffffff0,0xfffffff8,0xfffffffc,0xfffffffe,0xffffffff};

class BGPRouting : public Ipv4RoutingProtocol {

	public:
	BGPRouting();

	static TypeId GetTypeId (void);

	Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                                Socket::SocketErrno &sockerr);
    bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                      UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                      LocalDeliverCallback lcb, ErrorCallback ecb);

  	virtual void NotifyInterfaceUp (uint32_t interface);
  	virtual void NotifyInterfaceDown (uint32_t interface);
  	virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  	virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  	virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  	virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

	void AddNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, Ipv4Address nextHop, uint32_t interface);
    void AddNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface);
	void RemoveNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, Ipv4Address nextHop, uint32_t interface);
	void RemoveNetworkRouteTo (Ipv4Address network, Ipv4Mask networkPrefix, uint32_t interface);

	void setNlri(std::vector<Ptr<BGPRoute>> *m_nlri);

	uint32_t m_asn;

	private:
	std::vector<Ptr<BGPRoute>> *m_nlri;
	Ptr<Ipv4> m_ipv4;

};

}

#endif // BGP_ROUTING_H