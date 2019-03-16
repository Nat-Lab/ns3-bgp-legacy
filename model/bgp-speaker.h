/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_S_H
#define BGP_S_H

#define HOLD_TIMER 240
#define KEEPALIVE_TIMER 80

#include <algorithm>
#include <vector>
#include <iterator>

#include <arpa/inet.h>

#include "bgp-peer.h"
#include "bgp-route.h"
#include "bgp-routing.h"
#include "bgp-peerstatus.h"
#include "bgp-fragment.h"
#include "ns3/application.h"
#include "ns3/uinteger.h"
#include "ns3/object-vector.h"
#include "ns3/ptr.h"
#include "ns3/vector.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/net-device.h"

namespace ns3 {

class BGPSpeaker : public Application {

	public:
	static TypeId GetTypeId (void);	
	BGPSpeaker();
	virtual ~BGPSpeaker();
	void setPeers (std::vector<Ptr<BGPPeer>> peers);
	void setRoutes (std::vector<Ptr<BGPRoute>> routes);
	void DoClose (PeerStatus *ps);
	void DoConnect (PeerStatus *ps);
	std::vector<PeerStatus*> *getPeers(void);
	std::vector<Ptr<BGPRoute>> *getRoutes(void);
	uint32_t getAsn(void);

	protected:
	virtual void DoDispose (void);

	private:
	std::vector<Ptr<BGPPeer>> m_peers;
	std::vector<PeerStatus*> m_peer_status;
	std::vector<Ptr<BGPRoute>> m_nlri;
	uint32_t m_asn;
	Ptr<Socket> m_sock;
	Ptr<BGPRouting> m_routing;

	BGPFragments frags;
	int m_buf_frag_len;

	virtual void StartApplication(void);
	virtual void StopApplication(void);
	void HandleAccept (Ptr<Socket> socket, const Address &src);
	bool HandleRequest (Ptr<Socket> socket, const Address &src);
	void HandleRead (Ptr<Socket> sock);
	void HandleConnectFailed (Ptr<Socket> sock);
	bool SpeakerLogic (Ptr<Socket> sock, uint8_t** const buffer, Ipv4Address src_addr);

};

}

#endif /* BGP_S_H */

