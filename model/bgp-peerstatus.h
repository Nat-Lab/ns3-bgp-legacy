#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"

namespace ns3 {

typedef struct PeerStatus {
	int status; // 0: open sent, 1: open cfm, 2: established
	Ptr<Socket> socket;
	Ipv4Address addr;
	uint32_t asn; 
} PeerStatus;

}