/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Manet Compare 1: GAODV vs AODV with Energy & Mobility
 * Metrics: Throughput, PDR, Energy, Control Overhead
 */
#include "ns3/wifi-module.h"      // ← Critical for WifiHelper, YansWifi*, etc.
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"
#include "ns3/gaodv-v1-helper.h"   // ← GAODV helper (V1)
#include "ns3/aodv-module.h"       // ← AODV support
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ManetCompare1");

// ========== GLOBAL PACKET TRACKING FOR PDR ==========
// Track unique packet UIDs received at sink (handles multipath duplicates)
static std::set<uint64_t> g_receivedPacketUids;
static uint32_t g_totalSentPackets = 0;

// Callback when sender transmits a packet
void SenderTxCallback(Ptr<const Packet> packet)
{
    g_totalSentPackets++;
}

// Callback when WiFi PHY drops a packet
void PhyRxDropCallback(Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
{
    NS_LOG_UNCOND("PhyRxDrop at t=" << Simulator::Now().GetSeconds() 
                  << "s reason=" << reason << " packetSize=" << packet->GetSize());
}

// Callback when sink receives a packet (deduplicates by UID)
void SinkRxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        uint64_t uid = packet->GetUid();
        g_receivedPacketUids.insert(uid); // Automatically deduplicates
    }
}
// ========== END GLOBAL TRACKING ==========

class ManetCompare1
{
public:
    ManetCompare1();
    void Run();
    void CommandSetup(int argc, char** argv);

private:
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    void ReceivePacket(Ptr<Socket> socket);
    void CheckThroughput();

    uint32_t port{9};
    uint32_t bytesTotal{0};
    uint32_t packetsReceived{0};
    uint32_t packetsSent{0};  // Total packets sent by sources

    std::string m_CSVfileName{"manet-compare-1.csv"};
    int m_nSinks{10};  // Number of sink nodes (receivers) - default 10 for 50-node simulation
    std::string m_protocolName{"GAODV"};
    double m_txp{7.5};
    bool m_traceMobility{false};
    bool m_flowMonitor{false};

    // Metrics
    std::vector<double> m_initialEnergy;
    std::vector<double> m_finalEnergy;
    uint32_t m_rreqCount{0};
    uint32_t m_rrepCount{0};
    
    // Random number generator for application start times
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

ManetCompare1::ManetCompare1()
{
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

static inline std::string
PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;
    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();
    if (InetSocketAddress::IsMatchingType(senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        oss << " received one packet from " << addr.GetIpv4();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str();
}

// Consolidated ReceivePacket to handle both PDR (Global) and Throughput (Local)
void
ManetCompare1::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom(senderAddress)))
    {
        // 1. PDR Tracking (Global Unique UIDs)
        uint64_t uid = packet->GetUid();
        g_receivedPacketUids.insert(uid);

        // 2. Throughput Tracking (Local)
        bytesTotal += packet->GetSize();
        packetsReceived += 1;
        
        NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));
    }
}

void
ManetCompare1::CheckThroughput()
{
    double kbs = (bytesTotal * 8.0) / 1000;
    bytesTotal = 0;

    std::ofstream out(m_CSVfileName, std::ios::app);
    out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
        << m_nSinks << "," << m_protocolName << "," << m_txp << std::endl;
    out.close();
    packetsReceived = 0;
    Simulator::Schedule(Seconds(1.0), &ManetCompare1::CheckThroughput, this);
}

Ptr<Socket>
ManetCompare1::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, port);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&ManetCompare1::ReceivePacket, this));
    return sink;
}

void
ManetCompare1::CommandSetup(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
    cmd.AddValue("traceMobility", "Enable mobility tracing", m_traceMobility);
    cmd.AddValue("protocol", "Routing protocol (OLSR, AODV, DSDV, DSR, GAODV)", m_protocolName);
    cmd.AddValue("flowMonitor", "enable FlowMonitor", m_flowMonitor);
    cmd.Parse(argc, argv);

    std::vector<std::string> allowedProtocols{"OLSR", "AODV", "DSDV", "DSR", "GAODV"};
    if (std::find(allowedProtocols.begin(), allowedProtocols.end(), m_protocolName) == allowedProtocols.end())
    {
        NS_FATAL_ERROR("No such protocol: " << m_protocolName);
    }
}

int
main(int argc, char* argv[])
{
    ManetCompare1 experiment;
    experiment.CommandSetup(argc, argv);
    experiment.Run();
    return 0;
}

void
ManetCompare1::Run()
{
    // Reset global tracking variables for this run
    g_receivedPacketUids.clear();
    g_totalSentPackets = 0;
    
    Packet::EnablePrinting();

    // Initialize CSV
    std::ofstream out(m_CSVfileName);
    out << "SimulationSecond,Throughput_kbps,PacketsReceived,NumberOfSinks,RoutingProtocol,TransmissionPower" << std::endl;
    out.close();

    int nWifis = 50;  // Default: 50 nodes for full simulation
    double TotalTime = 500.0;  // Simulation duration - increased to allow all nodes to communicate
    std::string rate("2048bps");
    std::string phyMode("OfdmRate6Mbps");  // Match 802.11a - AODV test uses OfdmRate6Mbps
    
    // ========== Network connectivity parameters ==========
    // Use default WiFi TX power (no explicit setting) - matches AODV test
    // m_txp is set from command line or uses class default
    
    std::cout << "\n========== SIMULATION PARAMETERS ==========" << std::endl;
    std::cout << "Protocol: " << m_protocolName << std::endl;
    std::cout << "Nodes: " << nWifis << " (" << m_nSinks << " sinks, " << (nWifis - m_nSinks) << " sources)" << std::endl;
    std::cout << "WiFi: 802.11a, OfdmRate6Mbps, Default channel" << std::endl;
    std::cout << "Grid spacing: 120m (matching AODV test)" << std::endl;
    std::cout << "Application start: Random 100-101s" << std::endl;
    std::cout << "Simulation time: " << TotalTime << "s" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create(nWifis);

    // 🔋 Energy Source - DISABLED for now to test WiFi
    // BasicEnergySourceHelper energyHelper;
    // energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(500.0));
    // auto energySources = energyHelper.Install(adhocNodes);

    // 📶 Wi-Fi setup - MATCH AODV test configuration EXACTLY
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);  // AODV test uses 802.11a!
    
    YansWifiPhyHelper wifiPhy;
    
    // ========== CRITICAL: Match AODV test exactly ==========
    wifiPhy.DisablePreambleDetectionModel();
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");  // AODV test uses YansErrorRateModel
    
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();  // Use Default() like AODV test
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);
    
    // Add mac - MATCH AODV test exactly
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("OfdmRate6Mbps"),  // AODV test uses OfdmRate6Mbps
                                 "RtsCtsThreshold", StringValue("2200"));
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    // Enable PCAP tracing on all nodes for debugging
    wifiPhy.EnablePcap("gaodv-test", adhocDevices);
    
    // Add PhyRxDrop trace to diagnose reception failures
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
                                  MakeCallback(&PhyRxDropCallback));

    // 🚶 Mobility - Grid positioning MATCHING AODV test (120m spacing)
    MobilityHelper mobilityAdhoc;
    
    // Use GridPositionAllocator matching AODV test exactly
    mobilityAdhoc.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0),
                                      "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(120.0),  // AODV test uses 120m spacing
                                      "DeltaY", DoubleValue(0),
                                      "GridWidth", UintegerValue(nWifis),
                                      "LayoutType", StringValue("RowFirst"));
    
    mobilityAdhoc.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityAdhoc.Install(adhocNodes);

    // 🧭 Routing protocol setup
    AodvHelper aodv;
    OlsrHelper olsr;
    DsdvHelper dsdv;
    DsrHelper dsr;
    DsrMainHelper dsrMain;
    GaodvV1Helper gaodv;  // GAODV uses V1 implementation

    InternetStackHelper internet;
    Ipv4ListRoutingHelper list;

    if (m_protocolName == "OLSR")
    {
        list.Add(olsr, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "AODV")
    {
        list.Add(aodv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "DSDV")
    {
        list.Add(dsdv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (m_protocolName == "DSR")
    {
        internet.Install(adhocNodes);
        dsrMain.Install(dsr, adhocNodes);
        if (m_flowMonitor)
        {
            NS_FATAL_ERROR("Error: FlowMonitor does not work with DSR. Terminating.");
        }
    }
    else if (m_protocolName == "GAODV")
    {
        // GAODV: G-AODV flowchart with geometric routing and energy factors
        // Enable comprehensive logging for debugging PDR issue
        LogComponentEnable("Gaodvv1RoutingProtocol", LOG_LEVEL_ALL);
        LogComponentEnable("Gaodvv1RoutingProtocol", LOG_PREFIX_TIME);
        LogComponentEnable("Gaodvv1RoutingProtocol", LOG_PREFIX_NODE);
        LogComponentEnable("Gaodvv1RoutingProtocol", LOG_PREFIX_FUNC);
        
        gaodv.Set("Alpha", DoubleValue(0.6));
        gaodv.Set("Ethf", DoubleValue(0.2));
        gaodv.Set("MaxRange", DoubleValue(250.0));  // Matches TX power range
        gaodv.Set("InitialEnergy", DoubleValue(500.0));
        gaodv.Set("EvaluationWindow", TimeValue(Seconds(1.0)));  // Reduced from 2s to 1s
        gaodv.Set("W1", DoubleValue(0.25));
        gaodv.Set("W2", DoubleValue(0.25));
        gaodv.Set("W3", DoubleValue(0.25));
        gaodv.Set("W4", DoubleValue(0.25));
        list.Add(gaodv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else
    {
        NS_FATAL_ERROR("Unsupported protocol: " << m_protocolName);
    }

    // 🌐 IP addressing
    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces = addressAdhoc.Assign(adhocDevices);

    // 📤 Applications: sinks = first m_nSinks nodes, sources = next m_nSinks
    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < m_nSinks && (i + m_nSinks) < (int)adhocNodes.GetN(); i++)
    {
        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));
        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));
        onoff1.SetAttribute("Remote", remoteAddress);
        
        // Stagger start times: random between 100-101s to allow routing to establish
        // Use uniform random to avoid simultaneous starts
        double startTime = 100.0 + m_uniformRandomVariable->GetValue(0.0, 1.0);
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + m_nSinks));
        temp.Start(Seconds(startTime));
        temp.Stop(Seconds(TotalTime));
        
        // Connect TX callback to track sent packets
        for (uint32_t j = 0; j < temp.GetN(); j++)
        {
            Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication>(temp.Get(j));
            if (onoff)
            {
                onoff->TraceConnectWithoutContext("Tx", MakeCallback(&SenderTxCallback));
            }
        }
    }

    // Store initial energy before simulation - DISABLED
    std::vector<double> initialEnergy;
    // for (uint32_t i = 0; i < adhocNodes.GetN(); ++i)
    // {
    //     Ptr<ns3::energy::BasicEnergySource> es = energySources.Get(i)->GetObject<ns3::energy::BasicEnergySource>();
    //     if (es)
    //     {
    //         initialEnergy.push_back(es->GetInitialEnergy());
    //     }
    // }

    // ▶️ Run
    CheckThroughput();
    
    // Log node positions for debugging
    std::cout << "\n========== NODE POSITIONS AT START ==========" << std::endl;
    for (uint32_t i = 0; i < std::min(adhocNodes.GetN(), (uint32_t)10); ++i)
    {
        Ptr<MobilityModel> mobility = adhocNodes.Get(i)->GetObject<MobilityModel>();
        Vector pos = mobility->GetPosition();
        std::cout << "Node " << i << " (" << adhocInterfaces.GetAddress(i) << "): "
                  << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    std::cout << "... (" << adhocNodes.GetN() << " nodes total)" << std::endl;
    std::cout << "============================================\n" << std::endl;
    
    Simulator::Stop(Seconds(TotalTime));
    Simulator::Run();

    // ========== CALCULATE METRICS ==========
    std::cout << "\n===== FINAL RESULTS =====" << std::endl;
    std::cout << "Protocol: " << m_protocolName << std::endl;

    // PDR - using unique packet tracking (handles multipath duplicates)
    uint32_t uniqueReceived = g_receivedPacketUids.size();
    double pdr = (g_totalSentPackets > 0) ? 
                 static_cast<double>(uniqueReceived) / g_totalSentPackets : 0.0;
    
    std::cout << "\n--- Packet Delivery Ratio (PDR) ---" << std::endl;
    std::cout << "Sender Sent: " << g_totalSentPackets << " pkts" << std::endl;
    std::cout << "Sink Received (unique): " << uniqueReceived << " pkts" << std::endl;
    std::cout << "PDR: " << (pdr * 100.0) << "%" << std::endl;

    // Energy consumption - DISABLED
    double totalInitialEnergy = 0.0, totalFinalEnergy = 0.0;
    // for (uint32_t i = 0; i < adhocNodes.GetN(); ++i)
    // {
    //     totalInitialEnergy += initialEnergy[i];
    //     
    //     Ptr<ns3::energy::BasicEnergySource> es = energySources.Get(i)->GetObject<ns3::energy::BasicEnergySource>();
    //     if (es)
    //     {
    //         totalFinalEnergy += es->GetRemainingEnergy();
    //     }
    // }
    double energyConsumed = totalInitialEnergy - totalFinalEnergy;
    
    std::cout << "\n--- Energy Consumption ---" << std::endl;
    std::cout << "Total Initial Energy: " << totalInitialEnergy << " J" << std::endl;
    std::cout << "Total Consumed Energy: " << energyConsumed << " J" << std::endl;
    std::cout << "Total Residual Energy: " << totalFinalEnergy << " J" << std::endl;

    // Export results to CSV
    std::string resultsFile = "results_" + m_protocolName + ".csv";
    std::ofstream csv(resultsFile, std::ios::app);
    
    // Write header if file is new
    csv.seekp(0, std::ios::end);
    if (csv.tellp() == 0)
    {
        csv << "protocol,nodes,simtime,txPower,packetsSent,packetsReceived,pdr,"
            << "initialEnergy,consumedEnergy,residualEnergy" << std::endl;
    }
    
    csv << m_protocolName << ","
        << nWifis << ","
        << TotalTime << ","
        << m_txp << ","
        << g_totalSentPackets << ","
        << uniqueReceived << ","
        << pdr << ","
        << totalInitialEnergy << ","
        << energyConsumed << ","
        << totalFinalEnergy << std::endl;
    
    csv.close();
    std::cout << "\nResults saved to: " << resultsFile << std::endl;
    std::cout << "========================\n" << std::endl;

    Simulator::Destroy();
}
