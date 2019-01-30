/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-route.h"
#include "libbgp.h"
#include <stdio.h>

namespace ns3 {

TypeId BGPRoute::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::BGPRoute")
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

    return tid;
} 

BGPRoute::BGPRoute() {
	
}

Ipv4Address BGPRoute::getPrefix() {
    return m_prefix;
}

void BGPRoute::setPrefix(Ipv4Address prefix) {
    m_prefix = prefix;
}

uint8_t BGPRoute::getLength() {
    return m_prefix_len;
}

void BGPRoute::setLength(uint8_t len) {
    m_prefix_len = len;
}

LibBGP::BGPRoute* BGPRoute::toLibBGP() {
    auto route = new LibBGP::BGPRoute;
    route->prefix = htonl(m_prefix.Get());
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

bool BGPRoute::operator== (const BGPRoute& other) {
    return m_prefix == other.m_prefix && m_prefix_len == other.m_prefix_len; // && src_peer == other.src_peer;
}

bool BGPRoute::isSame (const BGPRoute& other) {
    return m_prefix == other.m_prefix && m_prefix_len == other.m_prefix_len &&
           m_as_path == other.m_as_path && src_peer == other.src_peer;
}

std::vector<uint32_t>* BGPRoute::getAsPath() {
    return &m_as_path;
}

void BGPRoute::setAsPath(std::vector<uint32_t> path) {
    m_as_path = path;
}

}

