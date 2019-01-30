#ifndef BGP_PS_H
#define BGP_PS_H

#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"

namespace ns3 {

class BGPSpeaker;

typedef struct PeerStatus {
	int status; // -1: down, 0: open sent, 1: open cfm, 2: established
	Ptr<Socket> socket;
	Ipv4Address addr;
	uint32_t asn;
	uint32_t dev_id;
	BGPSpeaker *speaker;
	EventId e_keepalive_sender;

	void HandleClose(Ptr<Socket> socket);
	void HandleConnect(Ptr<Socket> socket);
	void KeepaliveSenderStart(Time dt);
	void KeepaliveSenderStop();
	void KeepaliveSender(Time dt);
} PeerStatus;

}

#endif