/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "nlri.h"
#include "libbgp/src/libbgp.h"

namespace ns3 {

TypeId NLRI::GetTypeId (void) {
    return TypeId ("ns3::NLRI")
        .SetParent<Object> ()
        .SetGroupName("Internet")
        .AddConstructor<NLRI> ()
        .AddAttribute("Prefix", "Prefix",
                      Ipv4AddressValue(),
                      MakeIpv4AddressAccessor (&NLRI::m_prefix),
                      MakeIpv4AddressChecker ())
        .AddAttribute("Length", "Prefix Length",
                      UintegerValue(),
                      MakeUintegerAccessor (&NLRI::m_prefix_len),
                      MakeUintegerChecker<uint8_t> (0, 24));
} 

NLRI::NLRI() {
	
}

LibBGP::BGPRoute* NLRI::toRoute() {
    auto route = new LibBGP::BGPRoute;
    route->prefix = m_prefix.Get();
    route->length = m_prefix_len;

    return route; 
}

}

