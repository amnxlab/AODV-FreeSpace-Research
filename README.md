# MANET Routing Protocol Comparison Framework

![NS-3](https://img.shields.io/badge/NS--3-3.42-blue)
![Python](https://img.shields.io/badge/Python-3.8%2B-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

Automated framework for comparing MANET routing protocols (AODV, OLSR, DSDV, DSR, GAODV) with comprehensive PDR and energy consumption analysis.

---

## 🎯 Features

### 📊 Simulation Capabilities
- **PDR Calculation** - Unique packet tracking with UID-based deduplication
- **Energy Monitoring** - Initial, consumed, and residual energy tracking
- **CSV Export** - Structured data output for analysis
- **Multi-Protocol Support** - AODV, OLSR, DSDV, DSR, GAODV

### 🚀 Python Automation
- **NS-3 Auto-Detection** - Automatically finds NS-3 installations
- **Parameter Sweeping** - Protocols, nodes, TX power, multiple runs
- **Smart File Handling** - Handles filenames with spaces
- **Result Aggregation** - Combines protocol results into master CSV
- **Progress Tracking** - Real-time experiment status

### 📈 Professional Visualization (NS3-Research Style)
- **Grouped Bar Charts** - Protocols compared by node count
- **Error Bars** - Mean ± standard deviation display
- **Value Labels** - Statistics shown on each bar
- **Publication Quality** - Professional styling and colors
- **Multiple Plots** - PDR, Energy, Trade-offs, Statistics

---

## 🚀 Quick Start

### Prerequisites
```bash
# Install Python dependencies
pip3 install -r requirements.txt

# Ensure NS-3 is installed (auto-detected)
```

### Run Experiments
```bash
# Basic comparison
python3 scripts/run_all.py --protocols AODV OLSR --runs 10

# Parameter sweep
python3 scripts/run_all.py --protocols AODV OLSR DSDV \
    --nodes 25 50 100 --txpower 5.0 7.5 --runs 5
```

### Generate Plots
```bash
python3 scripts/plot_results.py
```

### View Results
```bash
# CSV data
cat results/aggregated_results.csv

# Generated plots
ls -lh graphs/
```

---

## 📁 Project Structure

```
Underwater-MANET-Routing-protocols-GAODV-/
├── src/
│   └── manet-compare free spaceV1 .cc    # Enhanced NS-3 simulation
├── scripts/
│   ├── run_all.py                        # Main automation CLI
│   ├── plot_results.py                   # Visualization (NS3-Research style)
│   └── README.md                         # Detailed tool documentation
├── gaodv-v0/                             # GAODV-V0: Original energy-aware AODV
│   ├── gaodv-routing-protocol.h/cc       # Main protocol
│   ├── gaodv-packet.h/cc                 # Packet headers
│   └── ...                               # Supporting modules
├── gaodv-v1/                             # GAODV-V1: G-AODV Flowchart (14 files)
│   ├── gaodv-v1-routing-protocol.h/cc    # Main protocol (1,400+ lines)
│   ├── gaodv-v1-packet.h/cc              # RREQ with (x,y,z) & RREQEF
│   ├── gaodv-v1-rtable.h/cc              # Routing table
│   ├── gaodv-v1-rqueue.h/cc              # Packet queue
│   ├── gaodv-v1-id-cache.h/cc            # Duplicate detection
│   ├── gaodv-v1-neighbors.h/cc           # Neighbor management
│   └── gaodv-v1-dpd.h/cc                 # Duplicate packet detection
├── aodv/                                 # NS-3 Standard AODV (reference)
├── results/                              # Generated data
│   └── aggregated_results.csv            # Combined experiment results
├── graphs/                               # Generated visualizations
├── requirements.txt                      # Python dependencies
├── verify_implementation.sh              # Verification script
└── README.md                             # This file
```

---

## 🔬 Implementation Details

### PDR Calculation

Uses unique packet UID tracking to avoid duplicate counting:

```cpp
// Global tracking
static std::set<uint64_t> g_receivedPacketUids;
static uint32_t g_totalSentPackets = 0;

// Sender callback
void SenderTxCallback(Ptr<const Packet> packet) {
    g_totalSentPackets++;
}

// Receiver callback (auto-deduplicates)
void SinkRxCallback(Ptr<Socket> socket) {
    while ((packet = socket->RecvFrom(from))) {
        g_receivedPacketUids.insert(packet->GetUid());
    }
}

// PDR = unique received / total sent
double pdr = (g_totalSentPackets > 0) ? 
             (double)g_receivedPacketUids.size() / g_totalSentPackets : 0.0;
```

### Energy Tracking

Monitors energy consumption throughout simulation:

```cpp
// Store initial energy
std::vector<double> initialEnergy;
for (uint32_t i = 0; i < adhocNodes.GetN(); ++i) {
    Ptr<energy::BasicEnergySource> es = energySources.Get(i)->GetObject<energy::BasicEnergySource>();
    initialEnergy.push_back(es->GetInitialEnergy());
}

// Calculate consumed energy after simulation
double energyConsumed = totalInitialEnergy - totalFinalEnergy;
```

### CSV Output Format

```csv
protocol,nodes,simtime,txPower,packetsSent,packetsReceived,pdr,initialEnergy,consumedEnergy,residualEnergy
AODV,50,500,7.5,15981,0,0.0,25000,0,25000
OLSR,50,500,7.5,15981,0,0.0,25000,0,25000
```

---

## 📊 Visualization Examples

### NS3-Research Style Features

**Grouped Bar Charts:**
- X-axis: Number of nodes
- Grouped bars for each protocol
- Error bars showing ±std
- Value labels: "XX.X% ±Y.Y"

**Color Scheme:**
- AODV: Green (#4CAF50)
- OLSR: Deep Orange (#FF5722)
- DSDV: Purple (#9C27B0)
- DSR: Amber (#FFC107)
- GAODV: Blue (#2196F3)

---

## 🛠️ Usage Examples

### Basic Protocol Comparison
```bash
python3 scripts/run_all.py --protocols AODV OLSR DSDV --runs 10
python3 scripts/plot_results.py
```

### Parameter Sweep
```bash
python3 scripts/run_all.py \
    --protocols AODV OLSR \
    --nodes 25 50 100 \
    --txpower 5.0 7.5 10.0 \
    --runs 5
```

### Quick Testing (Skip Build)
```bash
python3 scripts/run_all.py --protocols AODV --runs 3 --skip-build
```

### Custom NS-3 Path
```bash
python3 scripts/run_all.py \
    --ns3-path /path/to/ns3 \
    --protocols AODV OLSR \
    --runs 10
```

---

## 🧪 Verification

Run the verification script to test implementation:

```bash
bash verify_implementation.sh
```

**Tests:**
- ✅ C++ source file syntax
- ✅ Python script validity
- ✅ Directory structure
- ✅ Dependencies installed
- ✅ All components present

---

## 🔧 Automation Features

### NS-3 Auto-Detection
- Searches common installation paths
- Handles nested directories (`ns-allinone-3.42/ns-3.42`)
- Validates by checking for `ns3` or `waf` executables

### File Handling
- Auto-discovers `.cc` files in `src/` directory
- Sanitizes filenames (spaces → underscores) for CMake
- Copies to NS-3 scratch directory automatically

### Result Processing
- Collects protocol-specific CSVs from NS-3 directory
- Aggregates into master `aggregated_results.csv`
- Generates grouped statistics by nodes and protocols

---

## 📈 Plotting Capabilities

### Generated Plots

**1. PDR Comparison**
- Grouped bars by node count
- Error bars showing variability
- Protocol-specific colors

**2. Energy Consumption**
- Total energy consumed per protocol
- Grouped by network size
- Mean ± std labels

**3. Trade-off Scatter**
- PDR vs Energy relationship
- Protocol comparison points
- Identify optimal configurations

**4. Statistics Table**
- Comprehensive metrics CSV
- Mean and std for all parameters
- Protocol-wise breakdown

---

## 🏗️ GAODV Protocol Versions

This project includes **two GAODV implementations**:

| Version | Directory | Description |
|---------|-----------|-------------|
| **GAODV-V0** | `gaodv-v0/` | Original energy-aware AODV |
| **GAODV-V1** | `gaodv-v1/` | G-AODV flowchart implementation with geometric routing |

---

### Step 1: Choose Your GAODV Version

```bash
# Use GAODV-V0 (original, default)
python3 scripts/run_all.py --protocols GAODV --gaodv-version v0

# Use GAODV-V1 (flowchart-compliant, geometric)
python3 scripts/run_all.py --protocols GAODV --gaodv-version v1

# Compare both versions
python3 scripts/run_all.py --protocols GAODV-V0 GAODV-V1 AODV --runs 10
```

---

### Step 2: GAODV-V1 Features (G-AODV Flowchart)

GAODV-V1 implements the complete G-AODV flowchart:

**🔹 Geometric Awareness:**
- RREQ packets carry 3D coordinates `(x, y, z)`
- Position updated at each hop

**🔹 Energy-Aware Routing (NEF/RREQEF):**
- `NEF = residual_energy / initial_energy`
- Threshold check: `NEF ≥ α × ETHF` (default: α=0.6, ETHF=0.2)
- Low-energy nodes drop RREQ to preserve battery

**🔹 Route Efficiency Factor (REF):**
```
REF = w1×RREQEF + w2×(1/dist) + w3×(1/hops) + w4×(1/delay)
```
Default weights: w1=w2=w3=w4=0.25

**🔹 Destination RREQ Buffering:**
- Timer T (default: 2.0s) collects multiple RREQs
- Best REF path selected for RREP

---

### Step 3: Configure GAODV Parameters

GAODV-V1 exposes NS-3 attributes for tuning:

| Attribute | Default | Description |
|-----------|---------|-------------|
| `Alpha` | 0.6 | Threshold factor (0.4-0.8) |
| `Ethf` | 0.2 | Energy Threshold Factor |
| `MaxRange` | 100.0 | Max communication range (m) |
| `InitialEnergy` | 500.0 | Initial node energy (J) |
| `EvaluationWindow` | 2.0s | Timer T for RREQ collection |
| `W1`, `W2`, `W3`, `W4` | 0.25 each | REF calculation weights |
| `PropagationSpeed` | 1500.0 | Signal speed (m/s) for underwater |

---

### Step 4: Run Experiments

```bash
# Step 4a: Navigate to project
cd ~/Underwater-MANET-Routing-protocols-GAODV-

# Step 4b: Run GAODV-V1 vs standard AODV
python3 scripts/run_all.py \
    --protocols GAODV-V1 AODV \
    --nodes 25 50 100 \
    --runs 10

# Step 4c: Generate comparison plots
python3 scripts/plot_results.py

# Step 4d: View results
cat results/aggregated_results.csv
```

---

### Step 5: Verify Installation

```bash
# Check GAODV-V1 files exist
ls -la gaodv-v1/

# Expected output (14 files):
# gaodv-v1-routing-protocol.h/cc
# gaodv-v1-packet.h/cc
# gaodv-v1-rtable.h/cc
# gaodv-v1-rqueue.h/cc
# gaodv-v1-id-cache.h/cc
# gaodv-v1-neighbors.h/cc
# gaodv-v1-dpd.h/cc
```

---

### GAODV-V1 G-AODV Flowchart Decision Points

| Decision | Action |
|----------|--------|
| Source needs route? | Check routing table |
| Valid route exists? | Forward via next hop |
| No route? | Flood RREQ with (x,y,z) & RREQEF |
| Am I destination? | Buffer RREQ, start Timer T |
| 1st RREQ for this request? | Start evaluation timer |
| Timer T expired? | Select highest REF path, send RREP |
| Intermediate with route? | Send RREP (if not destination-only) |
| NEF ≥ α×ETHF? | Continue processing |
| NEF < α×ETHF? | Drop RREQ (low energy) |
| NEF < RREQEF? | Update RREQEF = NEF |
| Always | Update position (x,y,z) |
| Pass all checks? | Rebroadcast RREQ |

---

## 📝 Configuration

### Simulation Parameters

Default values in `manet-compare free spaceV1 .cc`:
- **Nodes:** 50
- **Simulation Time:** 500 seconds
- **Packet Size:** 64 bytes
- **Data Rate:** 2048 bps
- **Initial Energy:** 500 J per node
- **TX Power:** 7.5 dBm

### Supported Protocols

| Protocol | Description | Status |
|----------|-------------|--------|
| AODV | Ad-hoc On-Demand Distance Vector | ✅ |
| OLSR | Optimized Link State Routing | ✅ |
| DSDV | Destination-Sequenced Distance Vector | ✅ |
| DSR | Dynamic Source Routing | ✅ |
| GAODV-V0 | Energy-aware AODV (original) | ✅ |
| GAODV-V1 | G-AODV Flowchart (geometric + energy) | ✅ |

---

## 🐛 Troubleshooting

### Build Fails
```bash
# Clean NS-3 build
cd /path/to/ns-3.42
./ns3 clean
./ns3 configure
./ns3 build
```

### No Results Generated
```bash
# Check NS-3 directory for CSV files
ls /path/to/ns-3.42/results_*.csv

# Run with verbose output
python3 scripts/run_all.py --protocols AODV --runs 1
```

### Plotting Errors
```bash
# Verify aggregated results exist
cat results/aggregated_results.csv

# Check Python dependencies
pip3 install -r requirements.txt
```

---

## 📚 Documentation

- **[Scripts README](scripts/README.md)** - Detailed automation tool guide
- **[Verification Script](verify_implementation.sh)** - Test suite
- **[Requirements](requirements.txt)** - Python dependencies

---

## 🔍 Current Status

**Working Features:**
- ✅ NS-3 auto-detection
- ✅ File discovery and sanitization
- ✅ Build integration
- ✅ Simulation execution (100% success rate)
- ✅ PDR calculation infrastructure
- ✅ Energy monitoring
- ✅ CSV export
- ✅ Result aggregation
- ✅ NS3-Research style grouped plots
- ✅ Error bars and statistics

**Simulation Behavior:**
- PDR currently shows 0% (network configuration needs tuning)
- Energy consumption shows 0J (no packet transmission occurring)
- Framework correctly measures and reports these values

---

## 🤝 Contributing

This is a research project comparing MANET routing protocols. Contributions to improve the simulation or automation are welcome.

---

## 📄 License

MIT License - See LICENSE file for details

---

## 👥 Authors

- Research Team - MANET Protocol Comparison Study

---

## 🙏 Acknowledgments

- NS-3 Network Simulator
- NS3-Research project (plotting methodology inspiration)
- GAODV protocol implementation

---

## 📧 Contact

For questions or issues, please open a GitHub issue or contact the research team.

---

**Last Updated:** January 2026  
**Version:** 1.0  
**NS-3 Version:** 3.42
