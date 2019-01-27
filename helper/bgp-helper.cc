/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-helper.h"

namespace ns3 {

/* ... */

BGPHelper::BGPHelper (uint32_t myAsn) {
    m_factory.SetTypeId (BGPSpeaker::GetTypeId());
    SetAttribute ("ASN", UintegerValue(myAsn));
}

void BGPHelper::SetAttribute(std::string name, const AttributeValue &value) {
    m_factory.Set(name, value);
}

void BGPHelper::AddPeer (Ipv4Address addr, uint32_t asn) {
    Ptr<BGPPeer> peer = new BGPPeer;
    peer->setAddress(addr);
    peer->setAsn(asn);
    m_peers.push_back(peer);
}

void BGPHelper::AddRoute (Ipv4Address prefix, uint8_t len) {
    Ptr<BGPRoute> route = new BGPRoute;
    route->setLength(len);
    route->setPrefix(prefix);
    m_nlri.push_back(route);
}

ApplicationContainer BGPHelper::Install (Ptr<Node> nodeptr) const {
    return ApplicationContainer(InstallPriv(nodeptr));
}

ApplicationContainer BGPHelper::Install (std::string nodename) const {
    Ptr<Node> node = Names::Find<Node> (nodename);
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer BGPHelper::Install (NodeContainer nodecont) const {
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = nodecont.Begin(); i != nodecont.End(); ++i)
        apps.Add(InstallPriv (*i));
   
    return apps;
}

Ptr<Application> BGPHelper::InstallPriv (Ptr<Node> node) const {
    auto app = m_factory.Create<BGPSpeaker>();
    app->setPeers(m_peers);
    app->setRoutes(m_nlri);

    Ptr<Application> app_ = app;

    node->AddApplication(app_);

    return app_;
}

}