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

BGPFilterRulesSet BGPHelper::AddPeer (Ipv4Address addr, uint32_t asn, uint32_t dev, bool passive) {
    auto in_filter = new BGPFilterRules;
    auto out_filter = new BGPFilterRules;
    in_filter->default_op = BGPFilterOP::ACCEPT;
    out_filter->default_op = BGPFilterOP::ACCEPT;
    return AddPeer(addr, asn, dev, in_filter, out_filter, passive);
}

BGPFilterRulesSet BGPHelper::AddPeer (Ipv4Address addr, uint32_t asn, uint32_t dev, BGPFilterRules *in_filter, BGPFilterRules *out_filter, bool passive) {
    BGPFilterRulesSet set;
    set.in_filter = in_filter;
    set.out_filter = out_filter;

    auto peer = new PeerData;
    peer->m_peer_as = asn;
    peer->m_peer_addr = addr;
    peer->m_peer_dev_id = dev;
    peer->passive = passive;
    peer->in_filter = in_filter;
    peer->out_filter = out_filter;
    m_peers.push_back(peer);

    return set;
}

void BGPHelper::AddRoute (Ipv4Address prefix, uint8_t len, Ipv4Address nexthop, uint32_t dev, bool local) {
    auto route = new RouteData;
    route->m_prefix_len = len;
    route->m_prefix = prefix;
    route->m_nexthop = nexthop;
    route->m_dev_id = dev;
    route->local = local;
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
    std::vector<Ptr<BGPPeer>> peers(m_peers.size());
    std::transform(m_peers.begin(), m_peers.end(), peers.begin(), [&node](PeerData *p) {
        Ptr<BGPPeer> peer = new BGPPeer;
        peer->m_peer_as = p->m_peer_as;
        peer->m_peer_dev_id = p->m_peer_dev_id;
        peer->m_peer_addr = p->m_peer_addr;
        peer->passive = p->passive;
        peer->in_filter = p->in_filter;
        peer->out_filter = p->out_filter;
        return peer;
    });

    std::vector<Ptr<BGPRoute>> nlri(m_nlri.size());
    std::transform(m_nlri.begin(), m_nlri.end(), nlri.begin(), [&node](RouteData *r) {
        Ptr<BGPRoute> route = new BGPRoute;
        route->m_prefix = r->m_prefix;
        route->m_prefix_len = r->m_prefix_len;
        route->next_hop = r->m_nexthop;
        route->device = node->GetDevice(r->m_dev_id);
        route->local = r->local;
        return route;
    });

    auto app = m_factory.Create<BGPSpeaker>();
    app->setPeers(peers);
    app->setRoutes(nlri);

    Ptr<Application> app_ = app;

    node->AddApplication(app_);

    return app_;
}

}