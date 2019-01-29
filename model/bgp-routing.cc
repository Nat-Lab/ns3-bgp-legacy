#include "bgp-routing.h"
#define LOG_INFO(x) NS_LOG_INFO("[I " <<  Simulator::Now() << "] AS " << m_asn << ": " << x)
#define LOG_WARN(x) NS_LOG_WARN("[W " <<  Simulator::Now() << "] AS " << m_asn << ": " << x)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BGPRouting");
NS_OBJECT_ENSURE_REGISTERED(BGPRouting);

BGPRouting::BGPRouting () {}

TypeId BGPRouting::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::BGPRouting")
        .SetParent<Ipv4RoutingProtocol>()
        .SetGroupName ("Internet")
        .AddConstructor<BGPRouting>();

    return tid;
}

Ptr<Ipv4Route> BGPRouting::RouteOutput 
    (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
     Socket::SocketErrno &sockerr) {

    Ipv4Address destination = header.GetDestination ();

    LOG_INFO("routing: look up route-out for: " << destination);

    // TODO

    return 0;

}

bool BGPRouting::RouteInput 
    (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
     UnicastForwardCallback ucb, MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb) {

    Ipv4Address destination = header.GetDestination ();
    LOG_INFO("routing: look up route-in for: " << destination);

    // TODO

    return false;
}

void BGPRouting::NotifyInterfaceUp (uint32_t interface) {

}

void BGPRouting::NotifyInterfaceDown (uint32_t interface) {

}

void BGPRouting::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {

}

void BGPRouting::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {

}

void BGPRouting::SetIpv4 (Ptr<Ipv4> ipv4) {

}

void BGPRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {

}

}