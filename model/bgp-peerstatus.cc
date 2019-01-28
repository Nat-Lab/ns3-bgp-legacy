#include "bgp-peerstatus.h"
#include "bgp-speaker.h"
namespace ns3 {

void PeerStatus::HandleClose(Ptr<Socket> socket) {
	socket = 0;
	status = -1;
	KeepaliveSenderStop();
	speaker->DoClose(this);
}

void PeerStatus::HandleConnect(Ptr<Socket> socket) {
	status = 0;
	this->socket = socket;
	speaker->DoConnect(this);
}

void PeerStatus::KeepaliveSenderStop() {
	//if (e_keepalive_sender) 
	e_keepalive_sender.Cancel();
	//e_keepalive_sender = 0;
} 

void PeerStatus::KeepaliveSenderStart(Time dt) {
	//if (!e_keepalive_sender) 
	KeepaliveSender(dt);
} 

void PeerStatus::KeepaliveSender(Time dt) {
	auto keepalive = new LibBGP::BGPPacket;
    keepalive->type = 4;
    uint8_t *buffer = (uint8_t *) malloc(4096);
    int len = keepalive->write(buffer);
    socket->Send(buffer, len, 0);
    delete keepalive;
    delete buffer;
    e_keepalive_sender = Simulator::Schedule(dt, &PeerStatus::KeepaliveSender, this, dt);
}

}