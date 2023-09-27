/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"


// ---------------------- Task 2 -----------------------
//    UDP Echo                                 UDP Echo
//   Server(21)                                 Client
//       *                                        *
//       *                                        *
//  n0   n1   n2 ---------------------- n3   n4   n5  
//  |    |    |       point-to-point    |    |    |
//  ===========        192.168.3.0      ===========
//     CSMA                                CSMA
//  192.168.1.0                         192.168.2.0
//
// -----------------------------------------------------

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SecondScriptExample");

int
main(int argc, char* argv[])
{
    bool verbose = true;

    // Each CSMA bus has 3 nodes
    uint32_t nCsma = 3;
    // Node 1 should be an UDP echo server
    uint32_t nEchoServer = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);

    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    nCsma = nCsma == 0 ? 1 : nCsma;

    // Create 3 nodes in the first shared bus operating under CSMA (LAN)
    NodeContainer csmaNodes_1;
    csmaNodes_1.Create(nCsma);

    // Create 3 nodes in the second shared bus operating user CSMA (LAN)
    NodeContainer csmaNodes_2;
    csmaNodes_2.Create(nCsma);

    // 2 nodes in the point-to-point link (Node2 and Node3)
    NodeContainer p2pNodes;
    p2pNodes.Add(csmaNodes_1.Get(nCsma - 1));
    p2pNodes.Add(csmaNodes_2.Get(0));

    // Setting p2p link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install p2p link on p2pNodes
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    // Setting the first CSMA link
    CsmaHelper csma_1;
    csma_1.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma_1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

    // Install CSMA on csmaNodes_1
    NetDeviceContainer csmaDevices_1;
    csmaDevices_1 = csma_1.Install(csmaNodes_1);

    // Setting the second CSMA link
    CsmaHelper csma_2;
    csma_2.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma_2.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

    // Install CSMA on csmaNodes_2
    NetDeviceContainer csmaDevices_2;
    csmaDevices_2 = csma_2.Install(csmaNodes_2);

    // Stack all nodes for internet
    InternetStackHelper stack;
    stack.Install(csmaNodes_1);
    stack.Install(csmaNodes_2);

    // Setting Ip address to the p2p devices
    Ipv4AddressHelper address;
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    // Setting Ip address to the nodes in the first CSMA
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces_1;
    csmaInterfaces_1 = address.Assign(csmaDevices_1);

    // Setting Ip address to the nodes in the second CSMA
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces_2;
    csmaInterfaces_2 = address.Assign(csmaDevices_2);

    // UDP server porint(21) setting
    UdpEchoServerHelper echoServer(21);

    // Set server start time at 1s and stop time at 10s
    ApplicationContainer serverApps = echoServer.Install(csmaNodes_1.Get(nEchoServer));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // Set UDP client attribute in order to send 2 packets at 4s and 7s
    // through setting maxPackets to 2 and Interval to 3 seconds. 
    UdpEchoClientHelper echoClient(csmaInterfaces_1.GetAddress(nEchoServer), 21);
    echoClient.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    // Set client start time at 4s and stop time at 10s
    ApplicationContainer clientApps = echoClient.Install(csmaNodes_2.Get(2));
    clientApps.Start(Seconds(4.0));
    clientApps.Stop(Seconds(10.0));

    // Trigger router to build the routing database
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable packet tracing only in Nodes 2 and 4
    csma_1.EnablePcap("second-csma2", 2, 1, true);
    csma_1.EnablePcap("second-csma2", 4, 0, true);

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
