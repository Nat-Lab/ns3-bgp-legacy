#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/bgp-speaker.h"
#include "ns3/bgp-peer.h"
#include "ns3/bgp-route.h"
#include "ns3/bgp-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/tap-bridge-module.h"

using namespace ns3;

/*                    AS65002
 *                   /
 *  AS65000 - AS65001 
 *                   \
 *                    AS65003 - AS65004 - AS65005 (tap @ 10.1.1.6)
 * AS65000: 172.16.0.0/24
 * AS65004: 172.20.0.0/24
 */

int main () {
    LogComponentEnable("BGPSpeaker", LOG_LEVEL_ALL);
    LogComponentEnable("BGPRouting", LOG_LEVEL_ALL);

    GlobalValue::Bind("SimulatorImplementationType", 
        StringValue("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    NodeContainer n;
    n.Create(6);
    
    InternetStackHelper internet;
    internet.Install(n);

    CsmaHelper csma;
    NetDeviceContainer d = csma.Install(n);

    TapBridgeHelper tapBridge;
    tapBridge.SetAttribute("Mode", StringValue("ConfigureLocal"));
    tapBridge.SetAttribute("DeviceName", StringValue("tap-ns3bgp"));
    tapBridge.Install(n.Get(5), d.Get(5));

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(d);

    BGPHelper bgp0 (65000);
    BGPHelper bgp1 (65001);
    BGPHelper bgp2 (65002);
    BGPHelper bgp3 (65003);
    BGPHelper bgp4 (65004);

    bgp0.AddRoute(Ipv4Address("172.19.0.0"), 24, Ipv4Address("127.0.0.1"), 0); // r0 route: 172.16.0.0/24 via 127.0.0.1 dev lo
    bgp4.AddRoute(Ipv4Address("172.20.0.0"), 24, Ipv4Address("127.0.0.1"), 0); // r5 route: 172.20.0.0/24 via 127.0.0.1 dev lo

    bgp0.AddPeer(i.GetAddress(1), 65001, 1); // AddPeer(peer_address, peer_asn, interface)
    bgp1.AddPeer(i.GetAddress(0), 65000, 1);

    bgp1.AddPeer(i.GetAddress(2), 65002, 1);
    bgp2.AddPeer(i.GetAddress(1), 65001, 1);

    bgp1.AddPeer(i.GetAddress(3), 65003, 1);
    bgp3.AddPeer(i.GetAddress(1), 65001, 1);

    bgp3.AddPeer(i.GetAddress(4), 65004, 1);
    bgp4.AddPeer(i.GetAddress(3), 65003, 1);

    bgp4.AddPeer(i.GetAddress(5), 65005);

    auto a0 = bgp0.Install(n.Get(0));
    auto a1 = bgp1.Install(n.Get(1));
    auto a2 = bgp2.Install(n.Get(2));
    auto a3 = bgp3.Install(n.Get(3));
    auto a4 = bgp4.Install(n.Get(4));

    a0.Start(Seconds(0.0));
    a1.Start(Seconds(0.0));
    a2.Start(Seconds(0.0));
    a3.Start(Seconds(0.0));
    a4.Start(Seconds(0.0));
    a4.Stop(Seconds(5.0));

    csma.EnablePcapAll("BGPSpeaker");

    Simulator::Run();

    return 0;
}