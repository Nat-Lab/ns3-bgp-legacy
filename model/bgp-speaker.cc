/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-speaker.h"
#include "libbgp/src/libbgp.h"

namespace ns3 {

TypeId BGPSpeaker::GetTypeId (void) {
    return TypeId ("ns3::BGPSpeaker")
        .SetParent<Application> ()
        .SetGroupName("Internet")
        .AddConstructor<BGPSpeaker> ()
        .AddAttribute("Peers", "List of address of peers",
                      ObjectVectorValue(),
                      MakeObjectVectorAccessor(&BGPSpeaker::m_peers),
                      MakeObjectVectorChecker<BGPPeer> ())
        .AddAttribute("ASN", "My ASN",
                      UintegerValue(),
                      MakeUintegerAccessor(&BGPSpeaker::m_asn),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute("NRLI", "NRLI",
                      ObjectVectorValue(),
                      MakeObjectVectorAccessor(&BGPSpeaker::m_nlri),
                      MakeObjectVectorChecker<NLRI> ());
} 

BGPSpeaker::BGPSpeaker() {

}

}

