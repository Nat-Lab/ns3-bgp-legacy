/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "bgp-speaker.h"
#include "libbgp.h"
#define LOG_INFO(x) NS_LOG_INFO("[I " <<  Simulator::Now() << "] AS " << m_asn << ": " << x)
#define LOG_WARN(x) NS_LOG_WARN("[W " <<  Simulator::Now() << "] AS " << m_asn << ": " << x)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BGPSpeaker");
NS_OBJECT_ENSURE_REGISTERED(BGPSpeaker);

TypeId BGPSpeaker::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::BGPSpeaker")
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
    
    return tid;
} 

BGPSpeaker::BGPSpeaker() {
    
}

void BGPSpeaker::setPeers (std::vector<Ptr<BGPPeer>> peers) {
    m_peers = peers;
}

void BGPSpeaker::setRoutes (std::vector<Ptr<BGPRoute>> routes) {
    m_nlri = routes;
}

BGPSpeaker::~BGPSpeaker() {
    m_sock = 0;
}

void BGPSpeaker::DoDispose() {
    Application::DoDispose();
}

void BGPSpeaker::StartApplication () {
    if (m_sock == 0) {
        auto v4_stack = GetNode()->GetObject<Ipv4>();
        auto list_rp = CreateObject<Ipv4ListRouting> ();
        list_rp->AddRoutingProtocol(CreateObject<Ipv4StaticRouting>(), 10);
        m_routing = CreateObject<BGPRouting>();
        m_routing->m_asn = m_asn;
        list_rp->AddRoutingProtocol(m_routing, 5);
        v4_stack->SetRoutingProtocol(list_rp);

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

        std::for_each(m_peers.begin(), m_peers.end(), [this](Ptr<BGPPeer> peer) {
            auto e_peer = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&peer](PeerStatus *ps) {
                return (peer->getAddress()).IsEqual(ps->addr);
            });

            if (e_peer != m_peer_status.end()) return;

            Ptr<Socket> s = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            InetSocketAddress bind_to = InetSocketAddress(Ipv4Address::GetAny(), 179);
            s->Bind(bind_to);
            auto peer_addr = peer->getAddress();
            auto peer_asn = peer->getAsn();

            if(s->Connect(InetSocketAddress(peer->getAddress(), 179)) == -1) {   
                LOG_WARN("failed to Connect() to peer " << peer_addr << " (AS" << peer_asn << ")");
                return;
            }

            LOG_INFO("send OPEN to " << peer_addr << " (AS" << peer_asn << ")");

            PeerStatus *ps = new PeerStatus;
            ps->socket = s;
            ps->asn = peer_asn;
            ps->addr = peer_addr;
            ps->status = 0;
            ps->speaker = this;

            s->SetConnectCallback(
                MakeCallback(&PeerStatus::HandleConnect, ps),
                MakeCallback(&BGPSpeaker::HandleConnectFailed, this)
            );

            s->SetRecvCallback(MakeCallback(&BGPSpeaker::HandleRead, this));

            s->SetCloseCallbacks(
                MakeCallback(&PeerStatus::HandleClose, ps),
                MakeCallback(&PeerStatus::HandleClose, ps)
            );

            //m_peer_status.push_back(ps);
        });
    }
}

void BGPSpeaker::StopApplication () {
    if (m_sock != 0) {
        std::for_each(m_peer_status.begin(), m_peer_status.end(), [](PeerStatus *ps) {
            ps->socket->Close();
            ps->socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
            ps->KeepaliveSenderStop();
        });
        m_peer_status.erase(m_peer_status.begin(), m_peer_status.end());
        m_sock->Close();
        m_sock->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeNullCallback<void, Ptr<Socket>, const Address &> ()
        );
        m_sock = 0;
    }
}

void BGPSpeaker::DoClose (PeerStatus *ps) {
    //if (ps->status)

    LOG_INFO("session with AS" << ps->asn << " closed");

    auto lambda_find = [&ps](Ptr<BGPRoute> r) {
        return r->src_peer == ps->addr;
    };
    auto rs = std::find_if(m_nlri.begin(), m_nlri.end(), lambda_find);
    if (rs != m_nlri.end()) {
        LOG_INFO("NLRI from AS" << ps->asn << " non empty, sending withdraws");
        auto pkt_send = new LibBGP::BGPPacket;
        auto update_send = new LibBGP::BGPUpdateMessage;
        auto w_routes = new std::vector<LibBGP::BGPRoute*>;

        do {
            w_routes->push_back((*rs)->toLibBGP());
            LOG_INFO("send withdraw " << (*rs)->getPrefix() << "/" << (int) (*rs)->getLength());
        } while (
            (rs = std::find_if(std::next(rs), m_nlri.end(), lambda_find)) != m_nlri.end()
        );

        pkt_send->update = update_send;
        pkt_send->type = 2;
        update_send->withdrawn_routes = w_routes;

        uint8_t *buffer = (uint8_t *) malloc(4096);
        int len = pkt_send->write(buffer);

        std::for_each(m_peer_status.begin(), m_peer_status.end(), [&buffer, &len, &ps](PeerStatus *ep) {
            // send to every established peer.
            if (ep->status == 2 && !ep->addr.IsEqual(ps->addr)) ep->socket->Send(buffer, len, 0);
        });

        delete pkt_send;
        delete buffer;
    }

    auto to_remove = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&ps](PeerStatus *ps_) {
        return ps->addr.IsEqual(ps_->addr);
    });

    if (to_remove != m_peer_status.end()) m_peer_status.erase(to_remove);

    delete ps;
}

void BGPSpeaker::DoConnect (PeerStatus *ps) {
    Ipv4Address me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal();
    auto socket = ps->socket;

    auto send_msg = new LibBGP::BGPPacket;
    send_msg->type = 1;
    send_msg->open = new LibBGP::BGPOpenMessage(m_asn, 15, htonl(me.Get()));

    uint8_t *buffer = (uint8_t *) malloc(4096);
    int len = send_msg->write(buffer);
    socket->Send(buffer, len, 0);
    m_peer_status.push_back(ps);

    delete buffer;
    delete send_msg;
}

void BGPSpeaker::HandleConnectFailed (Ptr<Socket> socket) {
    LOG_WARN("failed to connect ssocket" << socket);
    // TODO
}

void BGPSpeaker::HandleAccept (Ptr<Socket> sock, const Address &src) {
    sock->SetRecvCallback(MakeCallback(&BGPSpeaker::HandleRead, this));
}

bool BGPSpeaker::HandleRequest (Ptr<Socket> socket, const Address &src) {
    auto src_addr = (InetSocketAddress::ConvertFrom(src)).GetIpv4();
    auto peer = std::find_if(m_peers.begin(), m_peers.end(), [&src_addr](Ptr<BGPPeer> peer) {
        return src_addr.IsEqual(peer->getAddress()); 
    });
    if (peer == m_peers.end()) return false; // reject if not in peer list

    auto e_peer = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *peer) {
        return peer->status == 2 && src_addr.IsEqual(peer->addr);
    });
    if (e_peer != m_peer_status.end()) return false; // reject if already established.

    return true;
}

bool BGPSpeaker::SpeakerLogic (Ptr<Socket> sock, uint8_t **buffer, Ipv4Address src_addr) {
    auto pkt = new LibBGP::BGPPacket();
    *buffer = pkt->read(*buffer);

    if (pkt->type == 1 && pkt->open) {
        auto open_msg = pkt->open;
        auto asn = open_msg->getAsn();
        LOG_INFO("OPEN from AS" << asn);
        auto peer = std::find_if(m_peers.begin(), m_peers.end(), [&asn](Ptr<BGPPeer> peer) {
            return peer->getAsn() == asn; 
        });

        if (peer == m_peers.end()) {
            LOG_WARN("Rejecting unknow Peer AS" << asn);
            return true;
        }

        if (!src_addr.IsEqual((*peer)->getAddress())) {
            LOG_WARN("Rejecting invlaid source address from AS" << asn <<
                        ". Want " << (*peer)->getAddress() << ", but saw " << src_addr);
            return true;
        }

        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *ps) {
            return ps->addr.IsEqual(src_addr);
        });

        if (ps != m_peer_status.end()) {
            if ((*ps)->status == 0) { // we init the conn, send keepalive, go to OPEN_CONFIRM
                (*ps)->socket = sock;
                (*ps)->asn = asn;
                (*ps)->addr = src_addr;
                (*ps)->status = 1;
                (*ps)->speaker = this;

                sock->SetCloseCallbacks(
                    MakeCallback(&PeerStatus::HandleClose, *ps),
                    MakeCallback(&PeerStatus::HandleClose, *ps)
                );

                auto reply_msg = new LibBGP::BGPPacket;
                reply_msg->type = 4;
                uint8_t *buffer = (uint8_t *) malloc(4096);
                int len = reply_msg->write(buffer);
                sock->Send(buffer, len, 0);

                LOG_INFO("session with AS" << asn << " entered OPEN_CONFIRM");

                delete buffer;
                delete reply_msg;
                delete pkt;

                return true;
                
            }

            if ((*ps)->status == 1) { // race condition where both peer init conn?
                LOG_WARN("session with AS" << asn << " in OPEN_CONFIRM but got another open");
            }

            if ((*ps)->status == 2) { // conn-collison resolution?
                LOG_WARN("session with AS" << asn << " already established but got another open, updaing sock.");
                (*ps)->socket = sock;

                delete pkt;
                return true;
            }

        } else { // peer init the conn, reply open, go to OPEN_CONFIRM
            Ipv4Address me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal(); 

            auto reply_msg = new LibBGP::BGPPacket;
            reply_msg->type = 1;
            reply_msg->open = new LibBGP::BGPOpenMessage(m_asn, 15, htonl(me.Get()));

            uint8_t *buffer = (uint8_t *) malloc(4096);
            int len = reply_msg->write(buffer);
            sock->Send(buffer, len, 0);

            PeerStatus *_ps = new PeerStatus;
            _ps->socket = sock;
            _ps->asn = asn;
            _ps->addr = src_addr;
            _ps->status = 1;
            _ps->speaker = this;

            sock->SetCloseCallbacks(
                MakeCallback(&PeerStatus::HandleClose, _ps),
                MakeCallback(&PeerStatus::HandleClose, _ps)
            );

            m_peer_status.push_back(_ps);

            delete buffer;
            delete reply_msg;
            delete pkt;

            return true;
        }
    }

    if (pkt->type == 2 && pkt->update) {
        auto me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal(); 
        auto update = pkt->update;
        auto as_path = update->getAsPath();
        auto routes_drop = update->withdrawn_routes;
        auto routes_add = update->nlri;
        auto next_hop = Ipv4Address(ntohl(update->getNexthop()));

        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *ps) {
            return src_addr.IsEqual(ps->addr);
        });

        if (ps == m_peer_status.end()) { // ???
            LOG_WARN("got a UPDATE out of nowhere (" << src_addr <<")");
            delete pkt;
            return true;
        }

        if ((*ps)->status == 0 || (*ps)->status == 1) { // ???
            LOG_WARN("got UPDATE from AS" << (*ps)->asn << " but session is not yet established");
            delete pkt;
            return true;
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
                return true;
            }
            std::copy(as_path->begin(), as_path->end(), std::ostream_iterator<uint32_t>(as_path_str, " "));
        }

        if (as_path) as_path->insert(as_path->begin(), m_asn);

        std::for_each(routes_drop->begin(), routes_drop->end(), [this](LibBGP::BGPRoute *r) {
            auto route = BGPRoute::fromLibBGP(r);
            auto pfx = route->getPrefix();
            auto len = route->getLength();
            LOG_INFO("got withdraw: " << pfx << "/" << (int) len);
            // TODO: route: remove from kernel table

            auto to_erase = std::find_if(m_nlri.begin(), m_nlri.end(), [&route](Ptr<BGPRoute> r) {
                return *route == *r;
            });

            if(to_erase != m_nlri.end()) {
                LOG_INFO("remove from nlri: " << pfx << "/" << (int) len);
                m_nlri.erase(to_erase);
            };

            
        });
        
        std::for_each(routes_add->begin(), routes_add->end(), [this, &as_path_str, &next_hop, &as_path, &src_addr](LibBGP::BGPRoute *r) {
            auto route = BGPRoute::fromLibBGP(r);
            auto pfx = route->getPrefix();
            auto len = route->getLength();
            // TODO: route: add to kernel table.

            LOG_INFO("got: " << pfx << "/" << (int) len << 
                        ", path: " << (as_path_str.str()).c_str() << ", nexthop: " << next_hop);

            auto br = BGPRoute::fromLibBGP(r);
            br->setAsPath(*as_path);
            br->src_peer = src_addr;
            br->next_hop = next_hop;
            m_nlri.push_back(br);
        });

        // Update forward
        
        auto pkt_send = new LibBGP::BGPPacket;
        auto update_send = new LibBGP::BGPUpdateMessage;
        
        update_send->withdrawn_routes = update->withdrawn_routes;
        update_send->nlri = update->nlri;
        if (as_path) {
            update_send->setNexthop(htonl(me.Get())); // in real world we don't always nexthop=self, but whatever.
            update_send->setAsPath(as_path, true);
        };
        update_send->setOrigin(0);
            
        pkt_send->update = update_send;
        pkt_send->type = 2;

        uint8_t *buffer = (uint8_t *) malloc(4096);
        int len = pkt_send->write(buffer);
        
        std::for_each(m_peer_status.begin(), m_peer_status.end(), [&buffer, &src_addr, &len](PeerStatus *ep) {
            // send to every established peer.
            if (ep->status == 2 && !ep->addr.IsEqual(src_addr)) ep->socket->Send(buffer, len, 0);
        });

        delete buffer;
        delete update_send;
        delete pkt_send;
        delete pkt;

        return true;
    }

    if (pkt->type == 3) {
        // TODO
    }

    if (pkt->type == 4) {
        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *ps) {
            return src_addr.IsEqual(ps->addr);
        });

        if (ps == m_peer_status.end()) { // wtf?
            LOG_WARN("got a KEEPALIVE out of nowhere (" << src_addr << ")");
            delete pkt;
            return true;
        }

        if ((*ps)->status == 0) { // wtf?
            LOG_WARN("got KEEPALIVE from AS" << (*ps)->asn << " but no OPEN");
            delete pkt;
            return true;
        }

        if ((*ps)->status == 1) { // in OPEN_CONFIRM, go to ESTABLISHED
            (*ps)->status = 2;
            LOG_INFO("session with AS" << (*ps)->asn << " established");
            (*ps)->KeepaliveSenderStart(Seconds(5.0));

            if (m_nlri.size() == 0) return true;

            LOG_INFO("NLRI non empty, sending update to AS" << (*ps)->asn);

            auto me = (((GetNode())->GetObject<Ipv4>())->GetAddress(1, 0)).GetLocal();
            std::for_each(m_nlri.begin(), m_nlri.end(), [this, &ps, &me, &sock](Ptr<BGPRoute> route) {
                auto pkt_send = new LibBGP::BGPPacket;
                auto update_send = new LibBGP::BGPUpdateMessage;

                auto asp = route->getAsPath();
                if (asp->size() == 0) asp->push_back(m_asn);

                update_send->setAsPath(asp, true);
                update_send->setOrigin(0);
                update_send->setNexthop(htonl(me.Get()));
                update_send->addPrefix(htonl((route->getPrefix().Get())), route->getLength(), false);

                LOG_INFO("send " << route->getPrefix() << "/" << (int) route->getLength() << " to AS" << (*ps)->asn);

                pkt_send->type = 2;
                pkt_send->update = update_send;

                uint8_t *buffer = (uint8_t *) malloc(4096);
                int len = pkt_send->write(buffer);
                sock->Send(buffer, len, 0);

                delete update_send;
                delete pkt_send;
                delete buffer;
            });

            delete pkt;
            return true;
        }

        if ((*ps)->status == 2) { // in ESTABLISHED
            // TODO: hold timer related stuff.
            return true;
        }
        
    }

    return false;

}

void BGPSpeaker::HandleRead (Ptr<Socket> sock) {
    Address from;
    uint8_t *buffer = (uint8_t *) malloc(65536);
    int sz = sock->RecvFrom(buffer, 65536, 0, from);
    auto src_addr = (InetSocketAddress::ConvertFrom(from)).GetIpv4();
    if (sz < 19) return;

    uint8_t *buffer_ptr = buffer;
    bool has_valid = true;

    while (buffer_ptr - buffer < sz && has_valid) {
        has_valid = SpeakerLogic(sock, &buffer_ptr, src_addr);
    }
   
   delete buffer;
}

}

