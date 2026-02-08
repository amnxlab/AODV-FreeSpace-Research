# GAODV: Energy-Aware AODV Routing Protocol for Free-Space MANETs

![NS-3](https://img.shields.io/badge/NS--3-3.42-blue)
![Python](https://img.shields.io/badge/Python-3.8%2B-green)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Version](https://img.shields.io/badge/Version-0.4-orange)

Implementation of **GAODV (Geometric and Energy-Aware AODV)** based on the **EDA-AODV** paper for terrestrial free-space MANETs.

> **Reference Paper:** EDA-AODV: Energy and Distance Aware AODV Routing Protocol

---

> [!NOTE]
> ## 📡 TX Range and Node Spacing Configuration
> 
> The simulation uses a **configurable grid topology**. Default values are now safe:
> 
> | Parameter | Default | Description |
> |-----------|---------|-------------|
> | `--txrange` | 150m | Transmission range (must exceed node spacing) |
> | `--node-spacing` | 50m | Distance between nodes in grid |
> | `--txpower` | 7.5 dBm | TX power (affects energy consumption) |
> 
> Higher TX power now consumes more energy (power-dependent model).
> 
> ```bash
> # Example: larger spacing requires longer range
> python3 scripts/run_all.py --protocols GAODV AODV --node-spacing 100 --txrange 150 --runs 5
> ```

---

## 📋 Table of Contents

1. [Quick Start](#-quick-start)
2. [Documentation](#-documentation)
3. [Web Dashboard](#-web-dashboard)
4. [Validation Scripts](#-validation-scripts)
5. [Test Cases](#-test-cases)
6. [Performance Metrics](#-performance-metrics)
7. [CLI Reference](#-cli-reference)
8. [EDA-AODV Parameters](#-eda-aodv-paper-parameters)
9. [GAODV Algorithm & Flowchart](#-gaodv-algorithm--flowchart)
10. [Project Structure](#-project-structure)
11. [Troubleshooting](#-troubleshooting)
12. [Version History](#-version-history)

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

## 📚 Documentation

### Release Notes (v0.4)

**[📖 RELEASE_v0.4.md](docs/RELEASE_v0.4.md)** - **Complete v0.4 reference** (1100+ lines)

This comprehensive document consolidates all v0.4 changes:
- ✅ **3 Critical Bug Fixes** (MaxRange, Energy Source Connection, WiFi TX Power)
- ✅ **100% Flowchart Validation** (All 11 EDA-AODV steps verified)
- ✅ **New Features** (Web Dashboard, Command Display, Validation Suite)
- ✅ **Migration Guide** from v0.3
- ✅ **Technical Implementation** details
- ✅ **Performance Results** and benchmarks

### Implementation Summary

**[📝 IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Development log

Detailed summary of v0.4 implementation session:
- WiFi TX Power fix (2 lines)
- Command display feature (5 files modified)
- Validation scripts (4 new files)
- Testing and verification results

### Technical Documentation

| Document | Topic | Lines |
|----------|-------|-------|
| [RELEASE_v0.4.md](docs/RELEASE_v0.4.md) | **Complete v0.4 guide** | 1100+ |
| [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) | Development session log | 291 |
| [batch-v0.3-fix.md](docs/batch-v0.3-fix.md) | v0.3 energy model changes | 200+ |
| [batch_v0.2_fix.md](docs/batch_v0.2_fix.md) | v0.2 PDR bug fixes | 150+ |

> **Quick Start:** Read [RELEASE_v0.4.md](docs/RELEASE_v0.4.md) for complete v0.4 overview.

---

## 🌐 Web Dashboard

A modern web-based dashboard is available for running simulations without using the command line.

### Start the Dashboard

```bash
python3 dashboard/start_dashboard.py
```

This will:
1. Start the HTTP server on port 8000
2. Open `http://localhost:8000` in your browser

### Features
- **Visual Parameter Configuration** - All CLI options in a clean UI
- **Command Display** - See exact NS-3 command executed (NEW in v0.4)
- **Real-time Console Output** - Live simulation progress
- **Results Viewer** - Browse and download CSV files
- **Modern Design** - Purple/violet gradient theme

> See [RELEASE_v0.4.md - Web Dashboard](docs/RELEASE_v0.4.md#web-dashboard-) for complete guide.

---

## 🧪 Validation Scripts

**NEW in v0.4:** Automated test suite to verify GAODV implementation correctness.

### Quick Validation

```bash
# Complete validation suite (5 minutes)
chmod +x scripts/validate_*.sh scripts/full_gaodv_validation.sh
./scripts/full_gaodv_validation.sh
```

### Individual Tests

```bash
# Test 1: Energy source connection
./scripts/validate_energy_connection.sh
# ✅ Checks: NEF calculations, energy depletion, EnergySourceContainer access

# Test 2: MaxRange parameter enforcement
./scripts/validate_maxrange.sh
# ✅ Checks: Distance filtering, MaxRange enforcement in routing decisions

# Test 3: RREQEF flowchart logic
./scripts/validate_rreqef_logic.sh
# ✅ Checks: RREQEF updates, position tracking, path selection
```

### Legacy Verification

```bash
# Basic component check (older script from v0.2-v0.3)
./verify_implementation.sh
# Checks: C++ source components, Python scripts, dependencies
```

**Expected Output:**
```
╔═══════════════════════════════════════════════════════════════╗
║    GAODV PROTOCOL - COMPLETE VALIDATION SUITE                 ║
╚═══════════════════════════════════════════════════════════════╝

[1/5] Testing Energy Consumption Variability... ✅ PASS
[2/5] Testing Energy Source Connection...       ✅ PASS
[3/5] Testing MaxRange Parameter...             ✅ PASS
[4/5] Testing RREQEF Flowchart Logic...         ✅ PASS
[5/5] Command Display Test                      ℹ️  Manual

╔═══════════════════════════════════════════════════════════════╗
║    ✅ VALIDATION COMPLETE                                      ║
╚═══════════════════════════════════════════════════════════════╝
```

> See [RELEASE_v0.4.md - Validation Suite](docs/RELEASE_v0.4.md#3-comprehensive-validation-suite-) for detailed documentation.

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

#### Test 4: TX Range vs Node Spacing
```bash
# Test different configurations of range and spacing
# Default: 50m spacing with 150m range (good connectivity)
python3 scripts/run_all.py --protocols GAODV AODV --runs 3

# Larger spacing requires longer range
python3 scripts/run_all.py --protocols GAODV AODV --node-spacing 100 --txrange 150 --runs 3
python3 scripts/run_all.py --protocols GAODV AODV --node-spacing 100 --txrange 200 --runs 3

# Dense network (closer nodes)
python3 scripts/run_all.py --protocols GAODV AODV --node-spacing 30 --txrange 100 --runs 3
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
| `--ethf` | 0.2 | `--ethf 0.3` |
| `--txpower` | 7.5 | `--txpower 10.0` |

### 📡 RF/Wireless & Topology Parameters

| Option | Default | Range | Description |
|--------|---------|-------|-------------|
| `--txRange` | **150m** | 50-500m | Transmission range (must exceed node spacing) |
| `--node-spacing` | **50m** | 10-200m | Distance between nodes in grid topology |
| `--txpower` | **7.5 dBm** | 0-20 dBm | TX power (affects range with Friis model & energy) |

> **💡 Tip:** TX range must be greater than node spacing for connectivity.  
> Higher TX power = longer range + more energy consumption.

### 🔋 Energy Model Parameters

| Option | Default | Unit | Description |
|--------|---------|------|-------------|
| `--initial-energy` | **20** | Joules | Initial energy per node |
| `--idle-current` | **0.0273** | Amps | Idle current (27.3mA) |
| `--rx-current` | **0.0313** | Amps | RX current (31.3mA) |
| `--tx-current-base` | **0.010** | Amps | Base TX current (10mA + power-dependent) |

> **⚡ Battery Life Formula:**
> ```
> Node lifetime (seconds) = Initial Energy (J) / (Idle Current (A) × 3.3V)
> Default: 20J / (0.0273A × 3.3V) ≈ 222 seconds
> ```
>
> To extend battery life for longer simulations:
> ```bash
> # Option A: Increase initial energy (500s simulation)
> python3 scripts/run_all.py --initial-energy 50 --simtime 500
>
> # Option B: Reduce currents (for ultra-low-power nodes)
> python3 scripts/run_all.py --idle-current 0.01 --rx-current 0.01 --tx-current-base 0.005
> ```

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
| Energy Threshold (ETHF) | **20%** | `--ethf 0.2` |
| Alpha (α) | **0.6** | (code: Alpha=0.6, range 0.4-0.8) |
| MAC Layer | 802.11a | (hardcoded) |
| TX Range | **150 m** | `--txRange 150` |
| Node Spacing | **50 m** | `--node-spacing 50` |

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
│     └─ YES → Select path with highest RREQEF (max energy)      │
│              Send RREP to source via best path                  │
└─────────────────────────────────────────────────────────────────┘
```

### Key Equations

#### Node Energy Factor (NEF)
$$\text{NEF} = \frac{E_{\text{residual}}}{E_{\text{initial}}}$$

#### RREQ Energy Factor Update (Min-Energy Path)
$$\text{RREQEF}^{\text{new}} = \min(\text{RREQEF}^{\text{current}}, \text{NEF}_{\text{node}})$$

> Only updated when `NEF < RREQEF` (node has less energy than path minimum)

#### Path Selection at Destination (Per Flowchart)
$$\text{Best Path} = \arg\max_{\text{paths}}(\text{RREQEF})$$

> **Simplified per flowchart:** Destination selects the path with **maximum RREQEF** - the path where the weakest node has the highest remaining energy. No weighted calculation needed.

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
├── README.md                             # This file - Quick start guide
├── IMPLEMENTATION_SUMMARY.md             # v0.4 development log
├── requirements.txt                      # Python dependencies
├── verify_implementation.sh              # Legacy verification script
│
├── src/
│   └── manet-compare-freespace-v1.cc    # NS-3 simulation (scratch/)
│
├── scripts/                              # Automation & validation
│   ├── run_all.py                        # Main automation tool
│   ├── plot_results.py                   # Result visualization
│   ├── validate_energy_connection.sh     # Test NEF calculations (NEW v0.4)
│   ├── validate_maxrange.sh              # Test MaxRange filtering (NEW v0.4)
│   ├── validate_rreqef_logic.sh          # Test RREQEF flowchart (NEW v0.4)
│   └── full_gaodv_validation.sh          # Complete test suite (NEW v0.4)
│
├── gaodv-v1/                             # GAODV protocol module (ns3/src/)
│   ├── gaodv-v1-routing-protocol.cc      # Core routing logic
│   ├── gaodv-v1-routing-protocol.h       # Header file
│   ├── gaodv-v1-packet.cc                # RREQ/RREP/RERR packets
│   ├── gaodv-v1-rtable.cc                # Routing table
│   ├── gaodv-v1-neighbors.cc             # Neighbor management
│   ├── gaodv-v1-helper.cc                # NS-3 helper class
│   └── CMakeLists.txt                    # Build configuration
│
├── dashboard/                            # Web UI (NEW v0.4)
│   ├── server.py                         # Flask backend
│   ├── start.py                          # One-click launcher
│   ├── public/index.html                 # Single-page app
│   └── api/                              # Backend modules
│       ├── simulation.py                 # Simulation runner
│       ├── parameter_validator.py        # Parameter validation
│       └── visualizer.py                 # Data visualization
│
├── docs/                                 # Documentation
│   ├── RELEASE_v0.4.md                   # Complete v0.4 guide (1100+ lines)
│   ├── batch-v0.3-fix.md                 # v0.3 energy model changes
│   └── batch_v0.2_fix.md                 # v0.2 PDR bug fixes
│
├── aodv/, dsdv/, dsr/                    # Standard protocol implementations
├── results/                              # Simulation output CSVs
└── test/                                 # NS-3 regression test captures
```

**Key Files:**
- **[RELEASE_v0.4.md](docs/RELEASE_v0.4.md)** - Complete v0.4 reference
- **[README.md](README.md)** - This quick start guide
- **[run_all.py](scripts/run_all.py)** - Main simulation automation tool
- **[full_gaodv_validation.sh](scripts/full_gaodv_validation.sh)** - Automated test suite
- **[EDA-AODVEnergyandDistanceAwareAODVRoutingProtocol.pdf](EDA-AODVEnergyandDistanceAwareAODVRoutingProtocol.pdf)** - Reference paper

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

### v0.4 (Current) - Production Ready Release
**Release Date:** February 8, 2026

#### Critical Bug Fixes (3)
- 🐛 **MaxRange Parameter Bug** - Now actively filters routes by distance (was defined but unused)
- 🐛 **Energy Source Connection** - Fixed `ComputeNef()` to access `EnergySourceContainer` (NEF now shows real values 0.8-0.95, not always 1.0)
- 🐛 **WiFi TX Power Configuration** - Aligned PHY TX power with energy model (was using NS-3 default ~16dBm instead of configured 7.5dBm)

#### New Features
- ✨ **Web Dashboard** - Modern UI at `http://localhost:5000` (port 5000, not 8000)
- ✨ **Command Display** - Shows exact NS-3 command in terminal and dashboard
- ✨ **Validation Suite** - 4 automated test scripts for protocol verification

#### Flowchart Validation
- ✅ **100% EDA-AODV Compliance** - All 11 flowchart steps implemented and verified
- ✅ **RREQEF Updates Working** - Energy-aware path selection now functional
- ✅ **Distance Filtering Active** - MaxRange enforced at intermediate nodes and destination

#### Documentation
- 📄 **[RELEASE_v0.4.md](docs/RELEASE_v0.4.md)** - Comprehensive 1100+ line consolidated guide
- 📄 **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Development session log
- 📄 Consolidated all bug fix docs, validation reports, and guides

#### Files Changed
- 7 modified files (NS-3 + Python + Dashboard)
- 4 new validation scripts
- ~400 lines added/modified total

**Status:** ✅ Production Ready - All core functionality validated

---

### v0.3 - Critical Bug Fixes & Energy Model Overhaul
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
