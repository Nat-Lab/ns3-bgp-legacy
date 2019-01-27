#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/bgp-speaker.h"
#include "ns3/bgp-peer.h"
#include "ns3/bgp-route.h"
#include "ns3/bgp-helper.h"
#include "ns3/ipv4-address.h"

using namespace ns3;

/*                    AS65002
 *                   /
 *  AS65000 - AS65001 
 *                   \
 *                    AS65003
 */

int main () {
    LogComponentEnable ("BGPSpeaker", LOG_LEVEL_ALL);

    NodeContainer n;
    n.Create(4);
    
    InternetStackHelper internet;
    internet.Install(n);

    CsmaHelper csma;
    NetDeviceContainer d = csma.Install(n);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(d);

    BGPHelper bgp0 (65000);
    BGPHelper bgp1 (65001);
    BGPHelper bgp2 (65002);
    BGPHelper bgp3 (65003);

    bgp0.AddRoute(Ipv4Address("172.16.0.0"), 24); // route: 172.16.0.0/24

    bgp0.AddPeer(i.GetAddress(1), 65001);
    bgp1.AddPeer(i.GetAddress(0), 65000);

    bgp1.AddPeer(i.GetAddress(2), 65002);
    bgp2.AddPeer(i.GetAddress(1), 65001);

    bgp2.AddPeer(i.GetAddress(3), 65003);
    bgp3.AddPeer(i.GetAddress(2), 65002);

    auto a0 = bgp0.Install(n.Get(0));
    auto a1 = bgp1.Install(n.Get(1));
    auto a2 = bgp2.Install(n.Get(2));
    auto a3 = bgp3.Install(n.Get(3));

    a0.Start(Seconds(0.0));
    a1.Start(Seconds(0.0));
    a2.Start(Seconds(0.0));
    a3.Start(Seconds(0.0));

    Simulator::Run();

    return 0;


}