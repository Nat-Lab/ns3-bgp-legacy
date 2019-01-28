#include "bgp-peerstatus.h"
#include "bgp-speaker.h"
namespace ns3 {

void PeerStatus::HandleClose(Ptr<Socket> socket) {
	socket = 0;
	status = -1;
	speaker->DoClose(this);
}

}