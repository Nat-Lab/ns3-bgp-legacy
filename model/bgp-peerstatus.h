#ifndef BGP_PS_H
#define BGP_PS_H

#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"

namespace ns3 {

class BGPSpeaker;

typedef struct PeerStatus {
	int status; // -1: down, 0: open sent, 1: open cfm, 2: established
	Ptr<Socket> socket;
	Ipv4Address addr;
	uint32_t asn;
	BGPSpeaker *speaker;

	void HandleClose(Ptr<Socket> socket);
	void HandleConnect(Ptr<Socket> socket);
} PeerStatus;

}

#endif