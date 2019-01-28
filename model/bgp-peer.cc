/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "bgp-peer.h"
#include "libbgp.h"

namespace ns3 {

TypeId BGPPeer::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::BGPPeer")
        .SetParent<Object> ()
        .SetGroupName("Internet")
        .AddConstructor<BGPPeer> ()
        .AddAttribute("Address", "Address of the peer",
                      Ipv4AddressValue(),
                      MakeIpv4AddressAccessor (&BGPPeer::m_peer_addr),
                      MakeIpv4AddressChecker ())
        .AddAttribute("ASN", "Peer ASN",
                      UintegerValue(),
                      MakeUintegerAccessor (&BGPPeer::m_peer_as),
                      MakeUintegerChecker<uint32_t> ());

    return tid;
} 

BGPPeer::BGPPeer() {
    
}

Ipv4Address BGPPeer::getAddress () {
    return m_peer_addr;
}

void BGPPeer::setAddress (Ipv4Address addr) {
    m_peer_addr = addr;
}

uint32_t BGPPeer::getAsn () {
    return m_peer_as;
}

void BGPPeer::setAsn (uint32_t asn) {
    m_peer_as = asn;
}

}

