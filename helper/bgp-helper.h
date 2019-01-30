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
#include "ns3/net-device.h"

namespace ns3 {

/* ... */

typedef struct PeerData {
    uint32_t m_peer_as;
    uint32_t m_peer_dev_id;
	Ipv4Address m_peer_addr;
} PeerData;

typedef struct RouteData {
    Ipv4Address m_prefix;
    Ipv4Address m_nexthop;
    uint8_t m_prefix_len;
	uint32_t m_dev_id;
} RouteData;

class BGPHelper {
    public:
    BGPHelper (uint32_t myAsn);
    void SetAttribute (std::string name, const AttributeValue &value);
    void AddPeer (Ipv4Address addr, uint32_t asn, uint32_t dev);
    void AddRoute (Ipv4Address prefix, uint8_t len, Ipv4Address nexthop, uint32_t dev);
    ApplicationContainer Install (Ptr<Node> nodeptr) const;
    ApplicationContainer Install (std::string nodename) const;
    ApplicationContainer Install (NodeContainer nodecont) const;

    private:
    
    std::vector<PeerData*> m_peers;
    std::vector<RouteData*> m_nlri;
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory;
};

}

#endif /* BGP_HELPER_H */

