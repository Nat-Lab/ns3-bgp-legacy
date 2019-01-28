/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef BGP_S_H
#define BGP_S_H

#include <algorithm>
#include <vector>
#include <iterator>

#include <arpa/inet.h>

#include "bgp-peer.h"
#include "bgp-route.h"
#include "bgp-peerstatus.h"
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

namespace ns3 {

class BGPSpeaker : public Application {

	public:
	static TypeId GetTypeId (void);	
	BGPSpeaker();
	virtual ~BGPSpeaker();
	void setPeers (std::vector<Ptr<BGPPeer>> peers);
	void setRoutes (std::vector<Ptr<BGPRoute>> routes);
	void DoClose (PeerStatus *ps);

	protected:
	virtual void DoDispose (void);

	private:
	std::vector<Ptr<BGPPeer>> m_peers;
	std::vector<PeerStatus*> m_peer_status;
	std::vector<Ptr<BGPRoute>> m_nlri;
	uint32_t m_asn;
	Ptr<Socket> m_sock;

	virtual void StartApplication(void);
	virtual void StopApplication(void);
	void HandleAccept (Ptr<Socket> socket, const Address &src);
	bool HandleRequest (Ptr<Socket> socket, const Address &src);
	void HandleRead (Ptr<Socket> sock);
	void HandleConnect (Ptr<Socket> sock);
	void HandleConnectFailed (Ptr<Socket> sock);
	void KeepaliveSenderStart(Ptr<Socket> socket, Time dt);
	bool SpeakerLogic (Ptr<Socket> sock, uint8_t **buffer, Ipv4Address src_addr);

};

}

#endif /* BGP_S_H */

