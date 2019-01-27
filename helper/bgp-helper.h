/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_HELPER_H
#define BGP_HELPER_H

#include "ns3/bgp-speaker.h"
#include "ns3/bgp-route.h"
#include "ns3/bgp-peer.h"

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/names.h"

namespace ns3 {

/* ... */

class BGPHelper {
    public:
    BGPHelper (uint32_t myAsn);
    void SetAttribute (std::string name, const AttributeValue &value);
    void AddPeer (Ipv4Address addr, uint32_t asn);
    void AddRoute (Ipv4Address prefix, uint8_t len);
    ApplicationContainer Install (Ptr<Node> nodeptr) const;
    ApplicationContainer Install (std::string nodename) const;
    ApplicationContainer Install (NodeContainer nodecont) const;

    private:
    
    std::vector<Ptr<BGPPeer>> m_peers;
    std::vector<Ptr<BGPRoute>> m_nlri;
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory;
};

}

#endif /* BGP_HELPER_H */

