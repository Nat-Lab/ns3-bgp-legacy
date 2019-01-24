/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-peer.h"
#include "libbgp/src/libbgp.h"

namespace ns3 {

TypeId BGPPeer::GetTypeId (void) {
    return TypeId ("ns3::Peer")
        .SetParent<Object> ()
        .SetGroupName("Internet")
        .AddConstructor<NLRI> ()
        .AddAttribute("Address", "Address of the peer",
                      Ipv4AddressValue(),
                      MakeIpv4AddressAccessor (&BGPPeer::m_peer_addr),
                      MakeIpv4AddressChecker ())
        .AddAttribute("ASN", "Peer ASN",
                      UintegerValue(),
                      MakeUintegerAccessor (&BGPPeer::m_peer_as),
                      MakeUintegerChecker<uint32_t> ());
} 

BGPPeer::BGPPeer() {
    
}

Ipv4Address* BGPPeer::getAddress () {
    return &m_peer_addr;
}

uint32_t BGPPeer::getAsn () {
    return m_peer_as;
}

}

