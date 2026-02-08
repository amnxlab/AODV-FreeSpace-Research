# GAODV: Energy-Aware AODV Routing Protocol for Free-Space MANETs

![NS-3](https://img.shields.io/badge/NS--3-3.42-blue)
![Python](https://img.shields.io/badge/Python-3.8%2B-green)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Version](https://img.shields.io/badge/Version-0.3-orange)

Implementation of **GAODV (Geometric and Energy-Aware AODV)** based on the **EDA-AODV** paper for terrestrial free-space MANETs.

> **Reference Paper:** EDA-AODV: Energy and Distance Aware AODV Routing Protocol

---

## 📋 Table of Contents

1. [Quick Start](#-quick-start)
2. [Test Cases](#-test-cases)
3. [Performance Metrics](#-performance-metrics)
4. [CLI Reference](#-cli-reference)
5. [EDA-AODV Parameters](#-eda-aodv-paper-parameters)
6. [GAODV Algorithm & Flowchart](#-gaodv-algorithm--flowchart)
7. [Project Structure](#-project-structure)
8. [Troubleshooting](#-troubleshooting)
9. [Version History](#-version-history)

---

## 🚀 Quick Start

> **Research Focus:** This implementation prioritizes **Packet Delivery Ratio (PDR)** and **Throughput** as primary performance metrics for routing protocol comparison.

### 1. Setup

```bash
# Install Python dependencies
pip3 install -r requirements.txt

# Copy files to NS-3
cp -r gaodv-v1/ /path/to/ns-3.42/src/gaodv-v1/
cp src/manet-compare-freespace-v1.cc /path/to/ns-3.42/scratch/

# Build NS-3
cd /path/to/ns-3.42 && ./ns3 build
```

### 2. Run Your First Simulation

```bash
# Simple test with GAODV (paper defaults)
python3 scripts/run_all.py --protocols GAODV --runs 1
```

---

## 🧪 Test Cases

### Ready-to-Run Commands

#### Test 1: Quick Validation (Small Network)
```bash
# 10 nodes, 100s simulation - fast validation
python3 scripts/run_all.py --protocols GAODV AODV --nodes 10 --simtime 100 --runs 1
```

#### Test 2: Paper Default Configuration
```bash
# EDA-AODV paper parameters: 50 nodes, 500s, 20J energy
python3 scripts/run_all.py --protocols GAODV AODV --nodes 50 --simtime 500 --runs 5
```

#### Test 3: Protocol Comparison (All Protocols)
```bash
# Compare GAODV with all standard protocols
python3 scripts/run_all.py --protocols GAODV AODV OLSR DSDV DSR --nodes 50 --runs 3
```

#### Test 4: TX Range Testing
```bash
# Test different transmission ranges (100m, 150m, 200m)
python3 scripts/run_all.py --protocols GAODV AODV --nodes 50 --runs 3 --txrange 100
python3 scripts/run_all.py --protocols GAODV AODV --nodes 50 --runs 3 --txrange 150
python3 scripts/run_all.py --protocols GAODV AODV --nodes 50 --runs 3 --txrange 200
```

#### Test 5: Network Scalability
```bash
# Test scalability: 25, 50, 100 nodes
python3 scripts/run_all.py --protocols GAODV AODV --nodes 25 50 100 --runs 5
```

#### Test 6: High Traffic Load
```bash
# 8 packets/second with 1024B packets
python3 scripts/run_all.py --protocols GAODV AODV --packet-rate 8 --packet-size 1024 --runs 5
```

#### Test 7: Extended Duration
```bash
# 1000 second simulation for energy depletion analysis
python3 scripts/run_all.py --protocols GAODV AODV --simtime 1000 --runs 3
```

#### Test 8: TX Power Variation
```bash
# Test different transmission power levels (deprecated - use --txrange instead)
python3 scripts/run_all.py --protocols GAODV AODV --txpower 5.0 7.5 10.0 --runs 3
```

#### Test 9: Full Benchmark Suite
```bash
# Complete benchmark for research paper
python3 scripts/run_all.py \
    --protocols GAODV AODV OLSR DSDV \
    --nodes 25 50 100 \
    --runs 10

# Generate visualization
python3 scripts/plot_results.py
```

### Direct NS-3 Commands

For debugging or single runs:

```bash
cd /path/to/ns-3.42

# Default parameters (50 nodes, 500s, GAODV)
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# AODV with 10 nodes for quick test
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=AODV --nWifis=10 --totalTime=100"

# Custom configuration
./ns3 run "scratch/manet-compare-freespace-v1 \
    --protocol=GAODV \
    --nWifis=50 \
    --nSinks=10 \
    --totalTime=500 \
    --txRange=100 \
    --packetRate=4 \
    --packetSize=512"

# All available options
./ns3 run "scratch/manet-compare-freespace-v1 --PrintHelp"
```

---

## 📊 Performance Metrics

### Primary Metrics (Research Focus)

| Metric | Description | Status |
|--------|-------------|--------|
| **PDR (%)** | Packet Delivery Ratio | ✅ Fully accurate |
| **Throughput (kbps)** | Data rate delivered to receivers | ✅ Validated |
| **Energy (J)** | Total consumed energy | ✅ Custom tracking |
| **Packets Sent/Received** | Absolute counts | ✅ Precise |

### Typical Results (50 nodes, 500s)

| Protocol | PDR | Throughput | Energy Consumed | Packets Sent | Packets Received |
|----------|-----|------------|-----------------|--------------|------------------|
| GAODV | 99.98% | 20.04 kbps | ~1000J | 19,575 | 19,571 |
| AODV | 100% | 20.04 kbps | ~1000J | 19,575 | 19,575 |
| OLSR | ~98% | ~20 kbps | ~1000J | 19,575 | ~19,180 |
| DSDV | ~96% | ~19.6 kbps | ~1000J | 19,575 | ~18,792 |

### Energy Tracking Details

**WifiRadioEnergyModel** (NS-3 built-in, PHY state-based tracking):
- Initial energy per node: 20J (configurable)
- Power consumption per state:
  - TX: 380mA (1.25W @ 3.3V)
  - RX: 313mA (1.03W @ 3.3V)
  - Idle: 273mA (0.90W @ 3.3V)
  - Sleep: 33mA (0.11W @ 3.3V)
- Tracks actual WiFi PHY state transitions
- Provides accurate per-activity energy consumption

> Energy model connects directly to WiFi PHY state machine for precise TX/RX/Idle/Sleep tracking based on actual protocol activity.

---

## 📖 CLI Reference

### Basic Syntax
```bash
python3 scripts/run_all.py [OPTIONS]
```

### Network Parameters

| Option | Example | Description |
|--------|---------|-------------|
| `--protocols` | `--protocols GAODV AODV` | Protocols to test |
| `--nodes` | `--nodes 50` or `--nodes 25 50 100` | Number of nodes |
| `--sinks` | `--sinks 10` | Number of sink/receiver nodes |
| `--runs` | `--runs 10` | Runs per configuration |

### Simulation Parameters (EDA-AODV Paper)

| Option | Default | Example |
|--------|---------|---------|
| `--simtime` | 500 | `--simtime 300` |
| `--initial-energy` | 20 | `--initial-energy 15` |
| `--packet-size` | 512 | `--packet-size 1024` |
| `--packet-rate` | 4 | `--packet-rate 8` |
| `--txpower` | 7.5 | `--txpower 10.0` |

### RF/Wireless Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--txRange` | 100 | Transmission range in meters |
| `--txpower` | 7.5 | Transmission power in dBm (if not using txRange) |

### System Options

| Option | Description |
|--------|-------------|
| `--ns3-path PATH` | Specify NS-3 installation path |
| `--skip-build` | Skip NS-3 rebuild (faster) |
| `--no-collect` | Don't move result files to results folder |

---

## 📐 EDA-AODV Paper Parameters

Based on **Table 1** from the EDA-AODV paper:

| Parameter | Value | CLI Option |
|-----------|-------|------------|
| Number of Nodes | 50 | `--nodes 50` |
| Simulation Time | 500s | `--simtime 500` |
| Initial Energy | **20 J** | `--initial-energy 20` |
| Packet Size | **512 B** | `--packet-size 512` |
| Packet Rate | **4 pkt/s** | `--packet-rate 4` |
| Energy Threshold (ETHF) | **10%** | (code: Ethf=0.1) |
| Alpha (α) | **0.6** | (code: Alpha=0.6) |
| MAC Layer | 802.11a | (hardcoded) |
| TX Range | **100 m** | `--txRange 100` |

---

## 📊 GAODV Algorithm & Flowchart

### G-AODV Flowchart Implementation

The implementation follows the G-AODV flowchart exactly:

```
┌─────────────────────────────────────────────────────────────────┐
│                         SOURCE NODE                              │
├─────────────────────────────────────────────────────────────────┤
│ 1. Source requires to transmit data                             │
│ 2. Check: Valid route exists?                                   │
│    ├─ YES → Transmit via next nodes in route                    │
│    └─ NO  → Start discovery: Flood RREQ with (x,y) & RREQEF     │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      INTERMEDIATE NODE                           │
├─────────────────────────────────────────────────────────────────┤
│ 3. 1st RREQ message? (duplicate check)                          │
│    └─ NO → RREQ message Dropped                                 │
│                                                                  │
│ 4. Valid route to destination exists?                           │
│    └─ YES → RREP to be sent to source                           │
│                                                                  │
│ 5. Calculate NEF: NEF >= ETHF?                                  │
│    └─ NO → RREQ message Dropped (low energy)                    │
│                                                                  │
│ 6. NEF < RREQEF?                                                │
│    ├─ YES → Update X,Y position AND RREQEF = NEF                │
│    └─ NO  → Update X,Y position only                            │
│                                                                  │
│ 7. Rebroadcast RREQ with reverse path                           │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      DESTINATION NODE                            │
├─────────────────────────────────────────────────────────────────┤
│ 8. 1st RREQ message? → Start Timer T                            │
│ 9. Set reverse path for each RREQ received                      │
│ 10. Timer expired?                                              │
│     └─ YES → Select path with maximum RREQEF                    │
│              Send RREP to source                                │
└─────────────────────────────────────────────────────────────────┘
```

### Key Equations

#### Node Energy Factor (NEF)
$$\text{NEF} = \frac{E_{\text{residual}}}{E_{\text{initial}}}$$

#### RREQ Energy Factor Update (Min-Energy Path)
$$\text{RREQEF}^{\text{new}} = \min(\text{RREQEF}^{\text{current}}, \text{NEF}_{\text{node}})$$

> Only updated when `NEF < RREQEF` (node has less energy than path minimum)

#### Best Path Selection (EDA-AODV Paper)
$$\text{Best Path} = \arg\max_{\text{paths}}(\text{RREQEF})$$

> "choose the path of maximum RqEF" - selects the path where the weakest node has the highest energy, maximizing route lifetime

#### Energy Threshold Check
$$\text{NEF}_{\text{node}} \geq \text{ETHF}$$

Where: ETHF = 0.1 (10% of initial energy)

> Nodes with NEF below threshold drop RREQ to preserve remaining energy

### Position Update Logic

| Condition | Position (X,Y) | RREQEF |
|-----------|----------------|--------|
| NEF < RREQEF | ✅ Updated | ✅ Updated to NEF |
| NEF >= RREQEF | ✅ Updated | ❌ Unchanged |

> Position is ALWAYS updated to current node's coordinates before rebroadcast

---

## 📁 Project Structure

```
AODV-FreeSpace-Research/
├── src/
│   └── manet-compare-freespace-v1.cc    # NS-3 simulation
├── scripts/
│   ├── run_all.py                        # Automation tool
│   └── plot_results.py                   # Visualization
├── gaodv-v1/                             # GAODV protocol module
│   ├── gaodv-v1-routing-protocol.cc      # Core routing logic
│   ├── gaodv-v1-routing-protocol.h       # Header file
│   ├── gaodv-v1-packet.cc                # RREQ/RREP packets
│   ├── gaodv-v1-rtable.cc                # Routing table
│   ├── gaodv-v1-neighbors.cc             # Neighbor management
│   └── CMakeLists.txt                    # Build config
├── docs/
│   └── batch_v0.2_fix.md                 # Bug fix documentation
├── results/                              # Output CSVs
├── graphs/                               # Generated plots
└── README.md
```

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| NS-3 not found | `--ns3-path /path/to/ns-3.42` |
| Build errors | `cd ns-3.42 && ./ns3 clean && ./ns3 build` |
| Zero PDR | Check TX range >= grid spacing (e.g., 150m for 120m spacing) |
| Missing modules | `pip3 install -r requirements.txt` |
| Low energy values | Normal - WifiRadioEnergyModel tracks actual PHY activity |

### Research Scope & Capabilities

**Primary Metrics (Fully Functional)**
- ✅ PDR (Packet Delivery Ratio) - highly accurate
- ✅ Throughput - validated against paper baselines  
- ✅ Packet counts - precise sender/receiver tracking
- ✅ **Energy consumption** - custom tracking implementation

**Energy Tracking Implementation**
- ✅ WifiRadioEnergyModel - NS-3 built-in PHY state-based tracking
- ✅ Accurate per-state power consumption (TX/RX/Idle/Sleep)
- ✅ Realistic power values: TX=380mA, RX=313mA, Idle=273mA, Sleep=33mA
- ✅ Connects directly to WiFi PHY state machine
- ✅ Shows total initial, consumed, and residual energy per simulation

> **Note:** v0.3 uses proper WifiRadioEnergyModel for accurate energy tracking. Previous versions used SimpleDeviceEnergyModel.

See [docs/batch-v0.3-fix.md](docs/batch-v0.3-fix.md) for details on energy model implementation.

---

## 📝 Version History

### v0.3 (Current) - Critical Bug Fixes & Energy Model Overhaul
- 🐛 **CRITICAL FIX**: InitialEnergy mismatch (500J→20J) - resolved 0% PDR
- 🐛 **CRITICAL FIX**: ComputeRef() now called for proper REF-based path selection
- 🐛 **CRITICAL FIX**: Alpha parameter integrated into NEF threshold
- ✅ **Energy Model**: WifiRadioEnergyModel (PHY state-based tracking)
- ✅ **Result Files**: Separate timestamped CSV per run (no aggregation)
- ✅ Alpha range corrected: 0.4-0.8 (was 0.5-0.7)
- 📄 Added `docs/batch-v0.3-fix.md` comprehensive documentation
- 🎯 **Metrics**: PDR (99.99%), Throughput (20kbps), Energy (1698J/500s for 100 nodes)

### v0.2.1 - Full Metrics Suite
- 🐛 **CRITICAL FIX**: Resolved 0% PDR issue (WiFi Energy Model conflict)
- ✅ **Path selection**: Max RREQEF (EDA-AODV paper Section 4.1)
- ✅ **Energy tracking**: Custom implementation bypassing PHY conflicts
- ✅ Corrected energy threshold: `NEF >= ETHF` (not `NEF >= α*ETHF`)
- ✅ 2D distance calculation (removed Z-axis for ground MANETs)
- ✅ WiFi config matching AODV regression test pattern
- ✅ Removed excessive debug output
- 📄 Added `docs/batch_v0.2_fix.md` documentation
- 🎯 **Metrics**: PDR (99.98%), Throughput (20kbps), Energy (~1000J/500s)

### v0.2 (January 2026)
- Route selection: max RqEF (EDA-AODV paper)
- Python CLI with all paper parameters
- Propagation speed: 3×10⁸ m/s (radio)
- Documentation overhaul

### v0.1 (January 2026)
- Initial GAODV-V1 implementation

---

## 📄 License

MIT License
