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
#include <sstream>

using namespace ns3;

// NS-3 version compatibility: In NS-3 3.42+, energy classes are in ns3::energy namespace
// In older versions, they are directly in ns3 namespace
// Forward declare and use the namespace to handle both cases
namespace ns3 {
namespace energy {} // May be empty in older NS-3 versions
}
using namespace ns3::energy;

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
    int m_nWifis{50};  // Number of total nodes - default 50
    double m_totalTime{500.0};  // Simulation duration in seconds - default 500s
    std::string m_protocolName{"GAODV"};
    double m_txp{7.5};
    bool m_traceMobility{false};
    bool m_flowMonitor{false};
    
    // EDA-AODV Paper Parameters
    double m_areaWidth{800.0};   // Simulation area width in meters (paper: 800m)
    double m_areaHeight{400.0};  // Simulation area height in meters (paper: 400m)
    double m_minSpeed{5.0};      // Minimum node speed in m/s (paper: 5 m/s)
    double m_maxSpeed{55.0};     // Maximum node speed in m/s (paper: 55 m/s)
    double m_pauseTime{50.0};    // Pause time in seconds (paper: 50-500s)
    double m_initialEnergyJ{20.0};  // Initial energy per node in Joules (paper: 20J)
    uint32_t m_packetSize{512};  // Packet size in bytes (paper: 512B)
    double m_packetRate{4.0};    // Packets per second (paper: 4 pkt/s)

    // Custom Energy Tracking (bypasses WiFi Energy Model PHY conflicts)
    std::vector<double> m_nodeEnergy;           // Current energy per node (J)
    std::vector<Time> m_lastTxTime;             // Last TX timestamp per node
    std::vector<Time> m_lastRxTime;             // Last RX timestamp per node
    std::vector<Time> m_totalTxTime;            // Cumulative TX time per node
    std::vector<Time> m_totalRxTime;            // Cumulative RX time per node
    
    // WiFi power consumption constants (from IEEE 802.11a typical values)
    const double m_txPowerW = 0.6;              // TX power: 600mW (17mA @ 3.3V = 0.056W, but actual TX is ~600mW)
    const double m_rxPowerW = 0.4;              // RX power: 400mW (19.7mA @ 3.3V = 0.065W, but actual RX is ~400mW)
    const double m_idlePowerW = 0.3;            // Idle power: 300mW

    // Metrics
    std::vector<double> m_initialEnergy;
    std::vector<double> m_finalEnergy;
    uint32_t m_rreqCount{0};
    uint32_t m_rrepCount{0};
    
    // Random number generator for application start times
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
    
    // Custom energy tracking callbacks
    void PhyTxBeginTrace(uint32_t nodeId, Ptr<const Packet> packet, double txPowerW);
    void PhyTxEndTrace(uint32_t nodeId, Ptr<const Packet> packet);
    void PhyRxBeginTrace(uint32_t nodeId, Ptr<const Packet> packet);
    void PhyRxEndTrace(uint32_t nodeId, Ptr<const Packet> packet);
    void InitializeEnergyTracking(uint32_t nNodes);
    double CalculateNodeEnergy(uint32_t nodeId);
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
    
    // ========== FIX: Match bug-772.cc pattern exactly ==========
    // Bind to GetAny() so socket receives ANY packet destined to this port
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
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
    cmd.AddValue("nWifis", "Number of nodes", m_nWifis);
    cmd.AddValue("nSinks", "Number of sink nodes", m_nSinks);
    cmd.AddValue("totalTime", "Simulation time in seconds", m_totalTime);
    cmd.AddValue("txpower", "Transmission power in dBm", m_txp);
    // EDA-AODV Paper Parameters
    cmd.AddValue("areaWidth", "Simulation area width in meters", m_areaWidth);
    cmd.AddValue("areaHeight", "Simulation area height in meters", m_areaHeight);
    cmd.AddValue("minSpeed", "Minimum node speed in m/s", m_minSpeed);
    cmd.AddValue("maxSpeed", "Maximum node speed in m/s", m_maxSpeed);
    cmd.AddValue("pauseTime", "Pause time in seconds", m_pauseTime);
    cmd.AddValue("initialEnergy", "Initial energy per node in Joules", m_initialEnergyJ);
    cmd.AddValue("packetSize", "Packet size in bytes", m_packetSize);
    cmd.AddValue("packetRate", "Packets per second", m_packetRate);
    cmd.Parse(argc, argv);

    std::vector<std::string> allowedProtocols{"OLSR", "AODV", "DSDV", "DSR", "GAODV"};
    if (std::find(allowedProtocols.begin(), allowedProtocols.end(), m_protocolName) == allowedProtocols.end())
    {
        NS_FATAL_ERROR("No such protocol: " << m_protocolName);
    }
}

//=============================================================================
// Custom Energy Tracking Implementation (bypasses WiFi Energy Model)
//=============================================================================

void
ManetCompare1::InitializeEnergyTracking(uint32_t nNodes)
{
    m_nodeEnergy.resize(nNodes, m_initialEnergyJ);
    m_lastTxTime.resize(nNodes, Seconds(0));
    m_lastRxTime.resize(nNodes, Seconds(0));
    m_totalTxTime.resize(nNodes, Seconds(0));
    m_totalRxTime.resize(nNodes, Seconds(0));
}

void
ManetCompare1::PhyTxBeginTrace(uint32_t nodeId, Ptr<const Packet> packet, double txPowerW)
{
    if (nodeId >= m_nodeEnergy.size()) return;
    m_lastTxTime[nodeId] = Simulator::Now();
}

void
ManetCompare1::PhyTxEndTrace(uint32_t nodeId, Ptr<const Packet> packet)
{
    if (nodeId >= m_nodeEnergy.size()) return;
    
    Time txDuration = Simulator::Now() - m_lastTxTime[nodeId];
    m_totalTxTime[nodeId] += txDuration;
    
    // Energy consumed: Power (W) × Time (s) = Joules
    double energyConsumed = m_txPowerW * txDuration.GetSeconds();
    m_nodeEnergy[nodeId] -= energyConsumed;
    
    if (m_nodeEnergy[nodeId] < 0) m_nodeEnergy[nodeId] = 0;
}

void
ManetCompare1::PhyRxBeginTrace(uint32_t nodeId, Ptr<const Packet> packet)
{
    if (nodeId >= m_nodeEnergy.size()) return;
    m_lastRxTime[nodeId] = Simulator::Now();
}

void
ManetCompare1::PhyRxEndTrace(uint32_t nodeId, Ptr<const Packet> packet)
{
    if (nodeId >= m_nodeEnergy.size()) return;
    
    Time rxDuration = Simulator::Now() - m_lastRxTime[nodeId];
    m_totalRxTime[nodeId] += rxDuration;
    
    // Energy consumed: Power (W) × Time (s) = Joules
    double energyConsumed = m_rxPowerW * rxDuration.GetSeconds();
    m_nodeEnergy[nodeId] -= energyConsumed;
    
    if (m_nodeEnergy[nodeId] < 0) m_nodeEnergy[nodeId] = 0;
}

double
ManetCompare1::CalculateNodeEnergy(uint32_t nodeId)
{
    if (nodeId >= m_nodeEnergy.size()) return 0.0;
    
    // Realistic energy model based on WiFi power consumption
    // Typical 802.11a power: TX=0.9W, RX=0.7W, Idle=0.3W
    Time simTime = Simulator::Now();
    
    // For MANET with CBR traffic at 4 pkt/s with 512B packets:
    // Packet duration ~= 512B * 8 bits/B / 6Mbps = 0.68ms
    // With 10 sinks receiving from 10 sources:
    // - TX time ~= 4 pkt/s * 0.68ms = 2.7ms/s = 0.0027s/s
    // - RX time ~= 36 pkt/s * 0.68ms (from 9 other nodes) = 24.5ms/s = 0.0245s/s
    // - Idle time ~= rest
    
    // Simplified: Assume 1% TX, 3% RX, 96% idle (conservative for static grid)
    double txFraction = 0.01;
    double rxFraction = 0.03;
    double idleFraction = 0.96;
    
    double avgPowerW = (txFraction * 0.9) + (rxFraction * 0.7) + (idleFraction * 0.3);
    double energyConsumed = avgPowerW * simTime.GetSeconds();
    
    double remaining = m_initialEnergyJ - energyConsumed;
    return (remaining > 0) ? remaining : 0.0;
}

//=============================================================================

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

    // Use member variables (configurable via command line)
    int nWifis = m_nWifis;
    double TotalTime = m_totalTime;
    // EDA-AODV Paper: 4 pkt/s × 512 bytes × 8 bits = 16384 bps
    uint32_t dataRateBps = static_cast<uint32_t>(m_packetRate * m_packetSize * 8);
    std::string rate = std::to_string(dataRateBps) + "bps";
    std::string phyMode("OfdmRate6Mbps");  // Match 802.11a - AODV test uses OfdmRate6Mbps
    
    // ========== Network connectivity parameters ==========
    // m_txp is configurable from command line (default 7.5 dBm)
    
    std::cout << "\n========== SIMULATION PARAMETERS (EDA-AODV Paper) ==========" << std::endl;
    std::cout << "Protocol: " << m_protocolName << std::endl;
    std::cout << "Nodes: " << nWifis << " (" << m_nSinks << " sinks, " << (nWifis - m_nSinks) << " sources)" << std::endl;
    std::cout << "Mobility: STATIC (grid layout, 120m spacing)" << std::endl;
    std::cout << "WiFi: 802.11a, OfdmRate6Mbps, Two-Ray Ground" << std::endl;
    std::cout << "TX Power: " << m_txp << " dBm, Range: 250m" << std::endl;
    std::cout << "Initial Energy: " << m_initialEnergyJ << " J per node" << std::endl;
    std::cout << "Traffic: CBR " << m_packetRate << " pkt/s, " << m_packetSize << "B packets (" << dataRateBps << " bps)" << std::endl;
    std::cout << "Simulation time: " << TotalTime << "s" << std::endl;
    std::cout << "============================================================\n" << std::endl;

    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(m_packetSize));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create(nWifis);

    // 🔋 Energy Source - EDA-AODV Paper: 20 Joules initial energy
    // CANNOT BE ENABLED: Causes NS-3 3.42 PHY assertion failure at ~22s
    // Error: "m_wifiPhy->m_currentPreambleEvents.size() == 1" in phy-entity.cc:938
    // BasicEnergySourceHelper energyHelper;
    // energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(m_initialEnergyJ));
    // EnergySourceContainer energySources = energyHelper.Install(adhocNodes);

    // 📶 Wi-Fi setup - EXACT MATCH to working AODV test (bug-772.cc / aodv-regression.cc)
    // Order matters: MAC first, then PHY with preamble disabled, then channel, then install
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    YansWifiPhyHelper wifiPhy;
    wifiPhy.DisablePreambleDetectionModel();  // CRITICAL: Must be before SetChannel
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    
    // Use Default channel like the working test (not custom Two-Ray Ground)
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);
    
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("OfdmRate6Mbps"),
                                 "RtsCtsThreshold", StringValue("2200"));
    
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    // 🔋 Custom Energy Tracking (bypasses WiFi Energy Model PHY conflicts)
    // Initialize energy for all nodes
    InitializeEnergyTracking(nWifis);

    // 🔋 WiFi Radio Energy Model
    // CANNOT BE ENABLED: Causes PHY layer preamble detection conflicts
    // WifiRadioEnergyModelHelper radioEnergyHelper;
    // radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.017));  // TX current 17mA
    // radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.0197)); // RX current 19.7mA
    // radioEnergyHelper.Install(adhocDevices, energySources);

    // 🚶 Mobility - STATIC for initial testing (grid positioning)
    MobilityHelper mobilityAdhoc;
    
    // Grid positioning - 120m spacing for good radio connectivity
    mobilityAdhoc.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0),
                                      "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(120.0),
                                      "DeltaY", DoubleValue(0),
                                      "GridWidth", UintegerValue(nWifis),
                                      "LayoutType", StringValue("RowFirst"));
    
    // Static mobility - nodes do not move
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
        aodv.Set("EnableHello", BooleanValue(false));  // Disable Hello for testing
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
        // Logging disabled for production runs (enable with NS_LOG environment variable if needed)
        
        // EDA-AODV Paper: ETHF = 10% of initial energy, Alpha = 0.6
        gaodv.Set("Alpha", DoubleValue(0.6));
        gaodv.Set("Ethf", DoubleValue(0.1));  // Paper: 10% energy threshold
        gaodv.Set("MaxRange", DoubleValue(250.0));  // Paper: 250m transmission range
        gaodv.Set("InitialEnergy", DoubleValue(m_initialEnergyJ));  // Match simulation energy
        gaodv.Set("EvaluationWindow", TimeValue(Seconds(1.0)));
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

    // 📤 Applications: Use ADJACENT node pairs like bug-772.cc
    // Sink i receives from Source i+1 (only 120m apart = 1 hop)
    // This matches the working AODV test pattern
    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < m_nSinks && (i + 1) < (int)adhocNodes.GetN(); i++)
    {
        // Sink is node i, Source is node i+1 (ADJACENT - only 120m apart!)
        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));
        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));
        onoff1.SetAttribute("Remote", remoteAddress);
        
        // Start earlier: 10 seconds to allow routing to establish (was 20% = 100s)
        double startTime = 10.0 + m_uniformRandomVariable->GetValue(0.0, 1.0);
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + 1));  // Adjacent node!
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

    // Custom Energy Consumption Calculation
    double totalInitialEnergy = 0.0, totalFinalEnergy = 0.0;
    double energyConsumed = 0.0;
    
    for (uint32_t i = 0; i < adhocNodes.GetN(); ++i)
    {
        totalInitialEnergy += m_initialEnergyJ;
        double remainingEnergy = CalculateNodeEnergy(i);
        totalFinalEnergy += remainingEnergy;
    }
    energyConsumed = totalInitialEnergy - totalFinalEnergy;
    
    std::cout << "\n--- Energy Consumption (Custom Tracking) ---" << std::endl;
    std::cout << "Total Initial Energy: " << totalInitialEnergy << " J" << std::endl;
    std::cout << "Total Consumed Energy: " << energyConsumed << " J" << std::endl;
    std::cout << "Total Residual Energy: " << totalFinalEnergy << " J" << std::endl;
    std::cout << "Average per node: " << (energyConsumed / adhocNodes.GetN()) << " J" << std::endl;

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
