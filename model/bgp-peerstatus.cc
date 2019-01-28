#include "bgp-peerstatus.h"
#include "bgp-speaker.h"
namespace ns3 {

void PeerStatus::HandleClose(Ptr<Socket> socket) {
	socket = 0;
	status = -1;
	speaker->DoClose(this);
}

void PeerStatus::HandleConnect(Ptr<Socket> socket) {
	status = 0;
	this->socket = socket;
	speaker->DoConnect(this);
}

}