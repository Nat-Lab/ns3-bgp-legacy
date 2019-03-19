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
    m_buf_frag_len = 0;
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

std::vector<PeerStatus*> *BGPSpeaker::getPeers(void) {
    return &m_peer_status;
}

std::vector<Ptr<BGPRoute>> *BGPSpeaker::getRoutes(void) {
    return &m_nlri;
}

uint32_t BGPSpeaker::getAsn(void) {
    return m_asn;
}

void BGPSpeaker::StartApplication () {
    if (m_sock == 0) {
        auto v4_stack = GetNode()->GetObject<Ipv4>();
        auto list_rp = CreateObject<Ipv4ListRouting> ();
        list_rp->AddRoutingProtocol(CreateObject<Ipv4StaticRouting>(), 10);
        m_routing = CreateObject<BGPRouting>();
        m_routing->m_asn = m_asn;
        m_routing->setNlri(&this->m_nlri);
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
            auto peer_addr = peer->getAddress();
            auto peer_asn = peer->getAsn();

            if (peer->passive) {
                LOG_INFO("session with " << peer_addr << " (AS" << peer_asn << ")" << " is set to passive, skip Connect().");
                return;
            }

            auto e_peer = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&peer](PeerStatus *ps) {
                return (peer->getAddress()).IsEqual(ps->addr);
            });

            if (e_peer != m_peer_status.end()) {
                LOG_INFO("session with " << peer_addr << " (AS" << peer_asn << ")" << " already exist, skip Connect().");
                return;
            }

            Ptr<Socket> s = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            InetSocketAddress bind_to = InetSocketAddress(Ipv4Address::GetAny(), 179);
            s->Bind(bind_to);
            

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
            ps->dev_id = peer->m_peer_dev_id;
            ps->in_filter = peer->in_filter;
            ps->out_filter = peer->out_filter;

            s->SetConnectCallback(
                MakeCallback(&PeerStatus::HandleConnect, ps),
                MakeCallback(&BGPSpeaker::HandleConnectFailed, this)
            );

            s->SetRecvCallback(MakeCallback(&BGPSpeaker::HandleRead, this));

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
        LibBGP::BGPPacket pkt_send;
        auto &update_send = pkt_send.update;
        auto &w_routes = update_send.withdrawn_routes;

        do {
            w_routes.push_back((*rs)->toLibBGP());
            LOG_INFO("send withdraw " << (*rs)->getPrefix() << "/" << (int) (*rs)->getLength());
        } while (
            (rs = std::find_if(std::next(rs), m_nlri.end(), lambda_find)) != m_nlri.end()
        );

        //pkt_send->update = update_send;
        pkt_send.type = 2;
        //update_send->withdrawn_routes = w_routes;

        uint8_t *buffer = (uint8_t *) malloc(4096);
        int len = pkt_send.write(buffer);

        std::for_each(m_peer_status.begin(), m_peer_status.end(), [&buffer, &len, &ps](PeerStatus *ep) {
            // send to every established peer.
            if (ep->status == 2 && !ep->addr.IsEqual(ps->addr)) ep->socket->Send(buffer, len, 0);
        });

        delete buffer;
    }

    auto to_remove = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&ps](PeerStatus *ps_) {
        return ps->addr.IsEqual(ps_->addr);
    });

    if (to_remove != m_peer_status.end()) m_peer_status.erase(to_remove);

    delete ps;
}

void BGPSpeaker::DoConnect (PeerStatus *ps) {
    Ipv4Address me = (((GetNode())->GetObject<Ipv4>())->GetAddress(ps->dev_id, 0)).GetLocal();
    auto socket = ps->socket;

    LibBGP::BGPPacket send_msg;
    send_msg.type = 1;
    send_msg.open = LibBGP::BGPOpenMessage(m_asn, HOLD_TIMER, htonl(me.Get()));

    uint8_t *buffer = (uint8_t *) malloc(4096);
    int len = send_msg.write(buffer);
    socket->Send(buffer, len, 0);
    m_peer_status.push_back(ps);

    delete buffer;
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

bool BGPSpeaker::SpeakerLogic (Ptr<Socket> sock, uint8_t** const buffer, Ipv4Address src_addr) {
    LibBGP::BGPPacket pkt;
    *buffer = pkt.read(*buffer);

    if (pkt.type == 1) {
        auto &open_msg = pkt.open;
        auto asn = open_msg.getAsn();
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

                LibBGP::BGPPacket reply_msg;
                reply_msg.type = 4;
                uint8_t *buffer = (uint8_t *) malloc(4096);
                int len = reply_msg.write(buffer);
                sock->Send(buffer, len, 0);

                LOG_INFO("session with AS" << asn << " entered OPEN_CONFIRM");

                delete buffer;

                return true;
            }

            if ((*ps)->status == 1) { // race condition where both peer init conn?
                LOG_WARN("session with AS" << asn << " in OPEN_CONFIRM but got another open");
            }

            if ((*ps)->status == 2) { // conn-collison resolution?
                LOG_WARN("session with AS" << asn << " already established but got another open, updating session socket.");
                (*ps)->socket->SetCloseCallbacks(
                    MakeNullCallback<void, Ptr<Socket>> (),
                    MakeNullCallback<void, Ptr<Socket>> ()
                );
                (*ps)->socket = sock;

                return true;
            }

        } else { // peer init the conn, reply open, go to OPEN_CONFIRM
            Ipv4Address me = (((GetNode())->GetObject<Ipv4>())->GetAddress((*peer)->m_peer_dev_id, 0)).GetLocal(); 

            LibBGP::BGPPacket reply_msg;
            reply_msg.type = 1;
            reply_msg.open = LibBGP::BGPOpenMessage(m_asn, HOLD_TIMER, htonl(me.Get()));

            uint8_t *buffer = (uint8_t *) malloc(4096);
            int len = reply_msg.write(buffer);
            sock->Send(buffer, len, 0);

            PeerStatus *_ps = new PeerStatus;
            _ps->socket = sock;
            _ps->asn = asn;
            _ps->addr = src_addr;
            _ps->status = 1;
            _ps->speaker = this;
            _ps->dev_id = (*peer)->m_peer_dev_id; 
            _ps->in_filter = (*peer)->in_filter;
            _ps->out_filter = (*peer)->out_filter;

            sock->SetCloseCallbacks(
                MakeCallback(&PeerStatus::HandleClose, _ps),
                MakeCallback(&PeerStatus::HandleClose, _ps)
            );

            m_peer_status.push_back(_ps);

            delete buffer;

            return true;
        }
    }

    if (pkt.type == 2) {
        auto &update = pkt.update;
        auto as_path = update.getAsPath();
        auto &routes_drop = update.withdrawn_routes;
        std::vector<LibBGP::BGPRoute> routes_add;
        auto next_hop = Ipv4Address(ntohl(update.getNexthop()));

        if (as_path && as_path->size() > 32) {
            LOG_WARN("as_path too long (> 32), routes will be discard.");
            return true;
        }

        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *ps) {
            return src_addr.IsEqual(ps->addr);
        });

        if (ps == m_peer_status.end()) { // ???
            LOG_WARN("got a UPDATE out of nowhere (" << src_addr <<")");
            return true;
        }

        if ((*ps)->status == 0 || (*ps)->status == 1) { // ???
            LOG_WARN("got UPDATE from AS" << (*ps)->asn << " but session is not yet established");
            return true;
        }

        if (update.nlri.size() > 0) std::copy_if(update.nlri.begin(), update.nlri.end(), std::back_inserter(routes_add), [&ps](LibBGP::BGPRoute rou) {
            return BGPFilterOP::ACCEPT == (*ps)->in_filter->apply(
                Ipv4Address(ntohl(rou.prefix)),
                Ipv4Mask(CIDR_MASK_MAP[rou.length])
            );
        });

        if (routes_add.size() == 0 && routes_drop.size() == 0) {
            LOG_INFO("No routes in this update or all of them has been rejected.");
            return true;
        }

        // Update processing

        std::stringstream as_path_str;
        if (as_path) {
            auto self = std::find_if(as_path->begin(), as_path->end(), [this, &ps](uint32_t asn) {
                return asn == m_asn && m_asn != (*ps)->asn;
            });
            if (self != as_path->end()) {
                // m_asn in as_path and not iBGP, ignore.
                return true;
            }
            std::copy(as_path->begin(), as_path->end(), std::ostream_iterator<uint32_t>(as_path_str, " "));
        }

        if (as_path) {
            /*if (m_asn != (*ps)->asn)*/ as_path->insert(as_path->begin(), m_asn);
            //else LOG_INFO("ibgp session, not inserting out AS to path.");
        }

        bool withdraw_accepted = false;

        if (routes_drop.size() > 0) std::for_each(routes_drop.begin(), routes_drop.end(), [this, &src_addr, &withdraw_accepted](LibBGP::BGPRoute r) {
            auto route = BGPRoute::fromLibBGP(r);
            auto pfx = route->getPrefix(); // menleak ?
            auto len = route->getLength();
            LOG_INFO("got withdraw: " << pfx << "/" << (int) len);

            auto to_erase = std::find_if(m_nlri.begin(), m_nlri.end(), [&route, &src_addr](Ptr<BGPRoute> r) {
                return *route == *r && r->src_peer == src_addr;
            });

            if (to_erase != m_nlri.end()) {
                LOG_INFO("nlri: remove " << pfx << "/" << (int) len);
                m_nlri.erase(to_erase);
                withdraw_accepted = true;
            } else LOG_INFO("nlri: no match " << pfx << "/" << (int) len);
        });
        
        if (routes_add.size() > 0) std::for_each(routes_add.begin(), routes_add.end(), [this, &ps, &as_path_str, &next_hop, &as_path, &src_addr, &sock](LibBGP::BGPRoute r) {
            auto route = BGPRoute::fromLibBGP(r); // memleak?
            auto pfx = route->getPrefix();
            auto len = route->getLength();

            LOG_INFO("got update: " << pfx << "/" << (int) len << 
                        ", path: " << (as_path_str.str()).c_str() << ", nexthop: " << next_hop);

            auto to_erase = std::find_if(m_nlri.begin(), m_nlri.end(), [&route, &src_addr](Ptr<BGPRoute> r) {
                return *route == *r && r->src_peer == src_addr;
            });

            bool do_add = false;

            if (to_erase != m_nlri.end()) {
                LOG_INFO("nlri: already exist: " << pfx << "/" << (int) len);
                auto old_sz = (*to_erase)->getAsPath()->size();
                auto new_sz = as_path->size();
                if (old_sz > new_sz) {
                    do_add = true;
                    LOG_INFO("nlri: removed old path with as_path len=" <<  old_sz << ": " << pfx << "/" << (int) len << " (new one has len=" << new_sz << ")");
                    m_nlri.erase(to_erase);
                } else LOG_INFO("nlri: keeping old path with as_path len=" <<  old_sz << ": " << pfx << "/" << (int) len << " (new one has len=" << new_sz << ")");
            } else do_add = true;

            
            if (do_add) {
                LOG_INFO("nlri: adding: " << pfx << "/" << (int) len);
                auto br = BGPRoute::fromLibBGP(r);
                br->setAsPath(*as_path);
                br->src_peer = src_addr;
                br->next_hop = next_hop;
                br->device = GetNode()->GetDevice((*ps)->dev_id);
                m_nlri.push_back(br);
            }
            
        });

        // Update forward
        
        LibBGP::BGPPacket pkt_send;
        auto &update_send = pkt_send.update; 
        
        if (withdraw_accepted)
            update_send.withdrawn_routes = update.withdrawn_routes; // FIXME
        
        update_send.nlri = routes_add;
        if (as_path) {
            update_send.setAsPath(*as_path, true);
        };
        update_send.setOrigin(0);
            
        //pkt_send->update = update_send;
        pkt_send.type = 2;

        uint8_t *buffer = (uint8_t *) malloc(4096);
        int len = pkt_send.write(buffer);
        
        std::for_each(m_peer_status.begin(), m_peer_status.end(), [&](PeerStatus *ep) {
            // skip src peer and non-established
            if (ep->status != 2 || ep->addr.IsEqual(src_addr)) return;

            if (update_send.nlri.size() != 0) {
                auto me = (((GetNode())->GetObject<Ipv4>())->GetAddress(ep->dev_id, 0)).GetLocal(); 
                LOG_INFO("using " << me << " as nexthop for AS " << ep->asn);
                update_send.setNexthop(htonl(me.Get()));
                // in real world we don't always nexthop=self, but whatever.

                len = pkt_send.write(buffer);
            }

            // send to every established peer.
            if (update_send.nlri.size() == 0) {
                if (withdraw_accepted && update_send.withdrawn_routes.size() > 0) {
                    ep->socket->Send(buffer, len, 0);
                }
                LOG_WARN("This update has no new NLRI, nor withdraw. Ignore.");
                return;
            }
            std::vector<LibBGP::BGPRoute> filtered_nlri;
            std::copy_if(routes_add.begin(), routes_add.end(), std::back_inserter(filtered_nlri), [&ep](LibBGP::BGPRoute rou) {
                return BGPFilterOP::ACCEPT == ep->out_filter->apply(
                    Ipv4Address(ntohl(rou.prefix)),
                    Ipv4Mask(CIDR_MASK_MAP[rou.length])
                );
            });

            update_send.nlri = filtered_nlri;
            len = pkt_send.write(buffer);

            if (update_send.nlri.size() == 0) {
                if (withdraw_accepted && update_send.withdrawn_routes.size() > 0) {
                    ep->socket->Send(buffer, len, 0);
                }
                LOG_WARN("Filtered NLRI has no new routes and withdraws. Ignore.");
            } else ep->socket->Send(buffer, len, 0);

        });

        delete buffer;
        return true;
    }

    if (pkt.type == 3) {
        // TODO
    }

    if (pkt.type == 4) {
        auto ps = std::find_if(m_peer_status.begin(), m_peer_status.end(), [&src_addr](PeerStatus *ps) {
            return src_addr.IsEqual(ps->addr);
        });

        if (ps == m_peer_status.end()) { // wtf?
            LOG_WARN("got a KEEPALIVE out of nowhere (" << src_addr << ")");
            return true;
        }

        if ((*ps)->status == 0) { // wtf?
            LOG_WARN("got KEEPALIVE from AS" << (*ps)->asn << " but no OPEN");
            return true;
        }

        if ((*ps)->status == 1) { // in OPEN_CONFIRM, go to ESTABLISHED
            (*ps)->status = 2;
            LOG_INFO("session with AS" << (*ps)->asn << " established");
            (*ps)->KeepaliveSenderStart(Seconds(KEEPALIVE_TIMER));

            if (m_nlri.size() == 0) return true;

            LOG_INFO("NLRI non empty, sending update to AS" << (*ps)->asn);

            auto me = (((GetNode())->GetObject<Ipv4>())->GetAddress((*ps)->dev_id, 0)).GetLocal();
            std::for_each(m_nlri.begin(), m_nlri.end(), [this, &ps, &me, &sock](Ptr<BGPRoute> route) {
                if (route->local) {
                    LOG_INFO("not sending local route: " << route->getPrefix() << "/" << (int) route->getLength());
                    return;
                }

                if (
                    BGPFilterOP::ACCEPT != (*ps)->out_filter->apply(
                        route->getPrefix(),
                        Ipv4Mask(CIDR_MASK_MAP[route->getLength()])
                    )
                ) {
                    LOG_INFO(route->getPrefix() << "/" << (int) route->getLength() << " filtered by out_filter. ");
                    return;
                }

                LibBGP::BGPPacket pkt_send;
                auto &update_send = pkt_send.update;

                auto asp = route->getAsPath();
                if (asp->size() == 0) asp->push_back(m_asn);

                update_send.setAsPath(*asp, true);
                update_send.setOrigin(0);
                update_send.setNexthop(htonl(me.Get()));
                update_send.addPrefix(htonl((route->getPrefix().Get())), route->getLength(), false);

                LOG_INFO("send " << route->getPrefix() << "/" << (int) route->getLength() << " to AS" << (*ps)->asn << " (nexthop = " << me << ")");

                pkt_send.type = 2;
                //pkt_send.update = update_send;

                uint8_t *buffer = (uint8_t *) malloc(4096);
                int len = pkt_send.write(buffer);
                sock->Send(buffer, len, 0);

                delete buffer;
            });

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
    uint8_t *buffer_ptr = buffer;
    int sz = sock->RecvFrom(buffer, 65536, 0, from);
    auto src_addr = (InetSocketAddress::ConvertFrom(from)).GetIpv4();
    auto frag = frags.getFragment(src_addr);

    bool has_valid = true;

    if (frag->size > 0) {
        if (sz == 19) {
            // this might not be a frag, but a KEEPALIVE being sent in race-condition.
            // dirty hack, maybe figure out a better way in the future.
            LibBGP::BGPPacket p;
            LibBGP::Parsers::parseHeader(buffer, &p);
            if (p.type == 4) {
                SpeakerLogic(sock, &buffer, src_addr);
                goto del_and_ret;
            }
        } else {
            memmove(buffer + frag->size, buffer, sz);
            memcpy(buffer, frag->buffer, frag->size);
            sz += frag->size;
            frag->clear();
            LOG_INFO("recv(): fragment from last time added to buffer.");
        }
    }
    
    if (sz < 19) goto del_and_ret;

    while (buffer_ptr - buffer < sz && has_valid) {
        auto buffer_left = sz - (buffer_ptr - buffer);
        if (buffer_left > 0 && buffer_left <= 19) { 
            if (buffer_left == 19) {
                LibBGP::BGPPacket p;
                LibBGP::Parsers::parseHeader(buffer_ptr, &p);
                if (p.type == 4) {
                    has_valid = SpeakerLogic(sock, &buffer_ptr, src_addr);
                    goto del_and_ret;
                }
            }
            LOG_INFO("recv(): TCP fragment needs to be deal with (left=" << buffer_left << ")");
            frag->set(buffer_ptr, buffer_left);
            goto del_and_ret;
        } else {
            LibBGP::BGPPacket p;
            LibBGP::Parsers::parseHeader(buffer_ptr, &p);
            if (p.length > buffer_left) {
                LOG_INFO("recv(): TCP fragment needs to be deal with (left=" << buffer_left << ", want=" << p.length << ")");
                frag->set(buffer_ptr, buffer_left);
                goto del_and_ret;
            }
        }
        has_valid = SpeakerLogic(sock, &buffer_ptr, src_addr);
    }

    del_and_ret:
    delete buffer;
    return;
}

}

