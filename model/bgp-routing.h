#ifndef BGP_ROUTING_H
#define BGP_ROUTING_H

#include "ns3/simulator.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/log.h"

namespace ns3 {

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

	uint32_t m_asn;

};

}

#endif // BGP_ROUTING_H