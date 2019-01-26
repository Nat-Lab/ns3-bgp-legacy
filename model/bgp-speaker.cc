/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-speaker.h"
#include "libbgp.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BGPSpeaker");
NS_OBJECT_ENSURE_REGISTERED(BGPSpeaker);

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
        .AddAttribute("NLRI", "NLRI",
                      ObjectVectorValue(),
                      MakeObjectVectorAccessor(&BGPSpeaker::m_nlri),
                      MakeObjectVectorChecker<BGPRoute> ());
} 

BGPSpeaker::BGPSpeaker() {
    
}

BGPSpeaker::~BGPSpeaker() {
    m_sock = 0;
}

void BGPSpeaker::DoDispose() {
    Application::DoDispose();
}

void BGPSpeaker::StartApplication () {
    if (m_sock == 0) {
        m_sock = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        InetSocketAddress listen = InetSocketAddress(Ipv4Address::GetAny(), 179);

        if (m_sock->Bind(listen) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        if (m_sock->Listen()) {
            NS_FATAL_ERROR("Failed to listen socket");
        }

        m_sock->SetAcceptCallback(
            MakeCallback(&BGPSpeaker::HandleRequest, this),
            MakeCallback(&BGPSpeaker::HandleAccept, this)
        );

    }
}

void BGPSpeaker::StopApplication () {
    if (m_sock != 0) {
        m_sock->Close();
        //m_sock->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void BGPSpeaker::HandleAccept (Ptr<Socket> sock, const Address &src) {
    sock->SetRecvCallback(MakeCallback(&BGPSpeaker::HandleRead, this));
}

bool BGPSpeaker::HandleRequest (Ptr<Socket> socket, const Address &src) {
    auto src_addr = Ipv4Address::ConvertFrom(src);
    auto peer = std::find_if(m_peers.begin(), m_peers.end(), [&src_addr](Ptr<BGPPeer> peer) {
        return src_addr.IsEqual(peer->getAddress()); 
    });
    if (peer == m_peers.end()) return false; // reject if not in peer list

    auto e_peer = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus peer) {
        return peer.status == 2 && src_addr.IsEqual(peer.addr);
    });
    if (e_peer != m_peer_status.end()) return false; // reject if already established.

    return true;
}

void BGPSpeaker::HandleRead (Ptr<Socket> sock) {
    Address from;
    uint8_t *buffer = (uint8_t *) malloc(4096);
    int sz = sock->RecvFrom(buffer, 4096, 0, from);
    auto src_addr = Ipv4Address::ConvertFrom(from);

    if (sz < 19) return;

    auto pkt = new LibBGP::BGPPacket(buffer);

    if (pkt->type == 1 && pkt->open) {
        auto open_msg = pkt->open;
        auto asn = open_msg->getAsn();
        NS_LOG_INFO("AS" << m_asn << ": OPEN from AS" << asn);
        auto peer = std::find_if(m_peers.begin(), m_peers.end(), [&asn](Ptr<BGPPeer> peer) {
            return peer->getAsn() == asn; 
        });

        if (peer == m_peers.end()) {
            NS_LOG_WARN("AS" << m_asn << ": Rejecting unknow Peer AS" << asn);
            return;
        }

        if (!src_addr.IsEqual((*peer)->getAddress())) {
            std::ostringstream want, saw;
            ((**peer).getAddress()).Print(want);
            src_addr.Print(saw);
            NS_LOG_WARN("AS" << m_asn << ": Rejecting invlaid source address from AS" << asn <<
                        ". Want " << (want.str()).c_str() << ", but saw " << (saw.str()).c_str());
            return;
        }

        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus ps) {
            return ps.addr.IsEqual(src_addr);
        });

        if (ps != m_peer_status.end()) {
            if (ps->status == 0) { // we init the conn, send keepalive, go to OPEN_CONFIRM
                ps->socket = sock;
                ps->asn = asn;
                ps->addr = src_addr;
                ps->status = 1;

                // TODO send KEEPALIVE

                NS_LOG_INFO("AS" << m_asn << ": session with AS" << asn << " entered OPEN_CONFIRM");
            }

            if (ps->status == 1) { // race condition where both peer init conn?
                NS_LOG_WARN("AS" << m_asn << ": session with AS" << asn << " in OPEN_CONFIRM but got another open");
            }

            if (ps->status == 2) { // wtf?
                NS_LOG_WARN("AS" << m_asn << ": session with AS" << asn << " already established but got another open");
                delete pkt;
                return;
            }

        } else { // peer init the conn, reply open, go to OPEN_CONFIRM
            Ipv4Address me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal(); 

            auto reply_msg = new LibBGP::BGPPacket;
            reply_msg->type = 1;
            reply_msg->open = new LibBGP::BGPOpenMessage(m_asn, 15, htonl(me.Get()));

            uint8_t *buffer = (uint8_t *) malloc(4096);
            int len = reply_msg->write(buffer);
            sock->Send(buffer, len, 0);

            PeerStatus _ps;
            _ps.socket = sock;
            _ps.asn = asn;
            _ps.addr = src_addr;
            _ps.status = 1;

            m_peer_status.push_back(_ps);

            delete buffer;
            delete reply_msg;
            delete pkt;

            return;
        }
    }

    if (pkt->type == 2 && pkt->update) {
        auto me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal(); 
        auto update = pkt->update;
        auto as_path = update->getAsPath();
        auto routes_drop = update->withdrawn_routes;
        auto routes_add = update->nlri;
        auto next_hop = Ipv4Address(update->getNexthop());

        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus ps) {
            return src_addr.IsEqual(ps.addr);
        });

        if (ps == m_peer_status.end()) { // ???
            NS_LOG_WARN("AS" << m_asn << ": got a UPDATE out of nowhere");
            delete pkt;
            return;
        }

        if (ps->status == 0 || ps->status == 1) { // ???
            NS_LOG_WARN("AS" << m_asn << ": got UPDATE from AS" << ps->asn << " but session is not yet established");
            delete pkt;
            return;
        }

        // Update processing

        std::stringstream as_path_str;
        if (as_path) {
            auto self = std::find_if(as_path->begin(), as_path->end(), [this](uint32_t asn) {
                return asn == m_asn;
            });
            if (self != as_path->end()) {
                // m_asn in as_path, ignore.
                delete pkt;
                return;
            }
            std::copy(as_path->begin(), as_path->end(), std::ostream_iterator<uint32_t>(as_path_str, " "));
        }

        std::for_each(routes_drop->begin(), routes_drop->end(), [this](LibBGP::BGPRoute *r) {
            std::ostringstream prefix;
            auto route = BGPRoute::fromLibBGP(r);
            auto pfx = route->getPrefix();
            auto len = route->getLength();
            // TODO: route: remove from kernel table.

            pfx.Print(prefix);
            NS_LOG_INFO("AS" << m_asn << ": withdraw: " << (prefix.str()).c_str() << "/" << len);
        });
        
        std::for_each(routes_add->begin(), routes_add->end(), [this, &as_path_str, &next_hop](LibBGP::BGPRoute *r) {
            std::ostringstream prefix, nxthop;
            auto route = BGPRoute::fromLibBGP(r);
            auto pfx = route->getPrefix();
            auto len = route->getLength();
            // TODO: route: add to kernel table.

            pfx.Print(prefix);
            next_hop.Print(nxthop);
            NS_LOG_INFO("AS" << m_asn << ": add: " << (prefix.str()).c_str() << "/" << len << 
                        ", path: " << (as_path_str.str()).c_str() << ", nexthtop: " <<
                        (nxthop.str()).c_str());
        });

        // Update forward

        if (as_path) as_path->push_back(m_asn);
        
        auto pkt_send = new LibBGP::BGPPacket;
        auto update_send = new LibBGP::BGPUpdateMessage;
            
        update_send->setNexthop(htonl(me.Get())); // in real world we don't always nexthop=self, but whatever.
        update_send->withdrawn_routes = update->withdrawn_routes;
        update_send->nlri = update->nlri;
        update_send->setAsPath(as_path, true);
        update_send->setOrigin(0);
            
        pkt_send->update = update;
        pkt_send->type = 2;

        uint8_t *buffer = (uint8_t *) malloc(4096);
        int len = pkt_send->write(buffer);
        
        std::for_each(m_peer_status.begin(), m_peer_status.end(), [&buffer, &src_addr, &len](PeerStatus ep) {
            // send to every established peer.
            if (ep.status == 2 && !ep.addr.IsEqual(src_addr)) ep.socket->Send(buffer, len, 0);
        });

        delete buffer;
        delete update_send;
        delete pkt_send;
        delete pkt;

        return;
    }

    if (pkt->type == 4) {
        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus ps) {
            return src_addr.IsEqual(ps.addr);
        });

        if (ps == m_peer_status.end()) { // wtf?
            NS_LOG_WARN("AS" << m_asn << ": got a KEEPALIVE out of nowhere");
            return;
        }

        if (ps->status == 0) { // wtf?
            NS_LOG_WARN("AS" << m_asn << ": session with AS" << ps->asn << " established");
        }

        if (ps->status == 1) { // in OPEN_CONFIRM, go to ESTABLISHED
            ps->status = 2;
            NS_LOG_INFO("AS" << m_asn << ": session with AS" << ps->asn << " established");
            // TODO: schedule regular keepalive
        }

        if (ps->status == 2) { // in ESTABLISHED
            // TODO: hold timer related stuff.
        }
        
    }
}

}

