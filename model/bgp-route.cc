/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-route.h"
#include "libbgp.h"

namespace ns3 {

TypeId BGPRoute::GetTypeId (void) {
    return TypeId ("ns3::BGPRoute")
        .SetParent<Object> ()
        .SetGroupName("Internet")
        .AddConstructor<BGPRoute> ()
        .AddAttribute("Prefix", "Prefix",
                      Ipv4AddressValue(),
                      MakeIpv4AddressAccessor (&BGPRoute::m_prefix),
                      MakeIpv4AddressChecker ())
        .AddAttribute("Length", "Prefix Length",
                      UintegerValue(),
                      MakeUintegerAccessor (&BGPRoute::m_prefix_len),
                      MakeUintegerChecker<uint8_t> (0, 24));
} 

BGPRoute::BGPRoute() {
	
}

Ipv4Address BGPRoute::getPrefix() {
    return m_prefix;
}

uint8_t BGPRoute::getLength() {
    return m_prefix_len;
}

LibBGP::BGPRoute* BGPRoute::toLibBGP() {
    auto route = new LibBGP::BGPRoute;
    route->prefix = m_prefix.Get();
    route->length = m_prefix_len;

    return route; 
}

Ptr<BGPRoute> BGPRoute::fromLibBGP(LibBGP::BGPRoute* route) {
    auto prefix = Ipv4Address(ntohl(route->prefix));
    uint8_t len = route->length;

    ObjectFactory m_factory;
    m_factory.SetTypeId(BGPRoute::GetTypeId());
    m_factory.Set("Prefix", Ipv4AddressValue(prefix));
    m_factory.Set("Length", UintegerValue(len));

    return m_factory.Create<BGPRoute>();
}

}

