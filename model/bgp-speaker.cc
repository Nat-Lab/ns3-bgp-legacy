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

BGPSpeaker::~BGPSpeaker() {
    m_sock = 0;
}

void BGPSpeaker::DoDispose() {
    Application::DoDispose();
}

void BGPSpeaker::StartApplication () {
    if (m_sock == 0) {
        m_sock = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        InetSocketAddress listen = InetSocketAddress(Ipv4Address::GetAny (), 179);

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
        m_sock->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void BGPSpeaker::HandleAccept (Ptr<Socket> sock, const Address &src) {
    sock->SetRecvCallback(MakeCallback(&BGPSpeaker::HandleRead, this));
}

bool BGPSpeaker::HandleRequest (Ptr<Socket> socket, const Address &src) {
    return true;
    // TODO
}

void BGPSpeaker::HandleRead (Ptr<Socket> sock) {
    Address from;
    uint8_t *buffer = (uint8_t *) malloc(4096);
    int sz = sock->RecvFrom(buffer, 4096, 0, from);

    if (sz < 19) return;

    // auto pkt = new LibBGP::BGPPacket(buffer);
    // TODO
}

}

