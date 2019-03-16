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

    Ipv4Address destination = header.GetDestination();

    LOG_INFO("routing: route-out requested for: " << destination);

    auto selected_route = lookup(destination);

    if (selected_route) {
        LOG_INFO("routing: nexthop of " << destination << " is at " << selected_route->next_hop);
        auto reply_rou = Create<Ipv4Route>();
        reply_rou->SetDestination(destination);
        reply_rou->SetGateway(selected_route->next_hop);
        reply_rou->SetOutputDevice(selected_route->device);
        reply_rou->SetSource(m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(selected_route->device), 0).GetLocal());
        return reply_rou;
    } else LOG_INFO("routing: route-out: we don't know how to get " << destination);

    return 0;

}

bool BGPRouting::RouteInput 
    (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
     UnicastForwardCallback ucb, MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb) {

    Ipv4Address destination = header.GetDestination();
    uint32_t iif = m_ipv4->GetInterfaceForDevice (idev); 
    LOG_INFO("routing: look up route-in for: " << destination);

    auto selected_route = lookup(destination);

    if (selected_route) {
        LOG_INFO("routing: nexthop of " << destination << " is at " << selected_route->next_hop);
        if (m_ipv4->IsDestinationAddress (destination, iif) ||
            selected_route->next_hop.CombineMask(CIDR_MASK_MAP[8]).IsEqual(Ipv4Address("127.0.0.0"))) {
            // next hop is loopback, treat as local.
            LOG_INFO("routing: nexthop is us, send to local.");
            lcb(p, header, 0);
            return true;
        }

        auto reply_rou = Create<Ipv4Route>();
        reply_rou->SetDestination(destination);
        reply_rou->SetGateway(selected_route->next_hop);
        reply_rou->SetOutputDevice(selected_route->device);
        reply_rou->SetSource(m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(selected_route->device), 0).GetLocal());
        ucb(reply_rou, p, header);
        
        return true;
    } else LOG_INFO("routing: route-in: we don't know how to get " << destination);

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
    m_ipv4 = ipv4;
}

void BGPRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {

}

void BGPRouting::setNlri (std::vector<Ptr<BGPRoute>> *m_nlri) {
    LOG_INFO("routing: installing new nlri.");
    std::for_each(m_nlri->begin(), m_nlri->end(), [this](Ptr<BGPRoute> r) {
        LOG_INFO("routing: table enrey: " << r->getPrefix());
    });
    this->m_nlri = m_nlri;
}

Ptr<BGPRoute> BGPRouting::lookup (const Ipv4Address &dest) const {
    int max_cidr = -1;
    Ptr<BGPRoute> selected_route = 0;

    std::for_each(m_nlri->begin(), m_nlri->end(), [this, &dest, &selected_route, &max_cidr](Ptr<BGPRoute> r) {
        auto cidr_len = r->getLength();
        auto mask = Ipv4Mask(CIDR_MASK_MAP[cidr_len]);
        if(dest.CombineMask(mask).IsEqual(r->getPrefix())) {
            if (max_cidr < cidr_len) { // more specific route found
                selected_route = r;
                max_cidr = cidr_len;
            }
            
            if (max_cidr == cidr_len) { // same mask, compare as_path
                if (r->getAsPath()->size() < selected_route->getAsPath()->size()) {
                    selected_route = r;
                }
            }
        }
    });

    return selected_route;
}

}