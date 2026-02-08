# GAODV: Energy-Aware AODV Routing Protocol for Free-Space MANETs

![NS-3](https://img.shields.io/badge/NS--3-3.42-blue)
![Python](https://img.shields.io/badge/Python-3.8%2B-green)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Version](https://img.shields.io/badge/Version-0.4-orange)

**Complete implementation of GAODV (Geometric and Energy-Aware AODV)** based on the EDA-AODV research paper for terrestrial free-space Mobile Ad-hoc Networks (MANETs).

> **Reference Paper:** EDA-AODV: Energy and Distance Aware AODV Routing Protocol

---

## 📋 Table of Contents

### Getting Started
1. [Quick Start](#-quick-start) - Install and run your first simulation
2. [Two Ways to Run](#-two-ways-to-run-simulations) - CLI tool vs Web Dashboard

### Using the Tools
3. [CLI Tool Guide](#-cli-tool-guide-run_allpy) - Complete command-line reference
4. [Web Dashboard Guide](#-web-dashboard-guide) - Browser-based interface
5. [Parameter Reference](#-parameter-reference) - All simulation options explained

### Understanding Results
6. [Performance Metrics](#-performance-metrics) - What the numbers mean
7. [EDA-AODV Paper Configuration](#-eda-aodv-paper-configuration) - Reference parameters

### Testing & Validation
8. [Validation Suite](#-validation-suite) - Verify implementation correctness
9. [Example Test Cases](#-example-test-cases) - Ready-to-run commands

### Technical Details
10. [GAODV Algorithm](#-gaodv-algorithm) - Flowchart and equations
11. [Project Structure](#-project-structure) - File organization
12. [Direct NS-3 Usage](#-direct-ns-3-usage) - Advanced debugging

### Reference
13. [Documentation](#-documentation) - Complete guides and release notes
14. [Troubleshooting](#-troubleshooting) - Common issues and solutions
15. [Version History](#-version-history) - Release changelog

---

## 🚀 Quick Start

Get GAODV running in 5 minutes.

### Prerequisites

- **NS-3.42** - Network simulator ([download here](https://www.nsnam.org/releases/))
- **Python 3.8+** - For automation scripts
- **Linux/macOS** - Primary development environment

### Installation

```bash
# 1. Install Python dependencies
pip3 install -r requirements.txt

# 2. Copy GAODV protocol module to NS-3
cp -r gaodv-v1/ /path/to/ns-3.42/src/gaodv-v1/

# 3. Copy simulation scenario to NS-3 scratch folder
cp src/manet-compare-freespace-v1.cc /path/to/ns-3.42/scratch/

# 4. Build NS-3 with GAODV module
cd /path/to/ns-3.42
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Your First Simulation

```bash
# Run a quick test (10 nodes, 100 seconds, 1 run)
python3 scripts/run_all.py --protocols GAODV --nodes 10 --simtime 100 --runs 1

# Expected output:
# ✓ PDR: ~99.9%
# ✓ Throughput: ~20 kbps
# ✓ Energy consumed: ~200J
# ✓ Results saved to: results/results_GAODV_session_*.csv
```

> **🎯 Research Focus:** This implementation prioritizes **Packet Delivery Ratio (PDR)**, **Throughput**, and **Energy Consumption** as primary performance metrics for routing protocol comparison.

---

## 🛠️ Two Ways to Run Simulations

Choose your preferred interface:

### Option 1: Command-Line Interface (CLI) ⌨️

**Best for:** Automation, batch testing, scripting, research workflows

```bash
# Basic usage
python3 scripts/run_all.py --protocols GAODV AODV --runs 5

# Multiple configurations
python3 scripts/run_all.py --protocols GAODV AODV OLSR --nodes 25 50 100 --runs 10
```

**Features:**
- ✅ Full parameter control via command-line flags
- ✅ Batch testing multiple protocols/configurations
- ✅ Integration with shell scripts and automation
- ✅ Direct run_all.py command display
- ✅ Perfect for reproducible research

👉 See [CLI Tool Guide](#-cli-tool-guide-run_allpy) for complete reference

### Option 2: Web Dashboard 🌐

**Best for:** Interactive exploration, parameter tuning, visual feedback

```bash
# Start the dashboard server
python3 dashboard/start.py

# Browser opens automatically at: http://localhost:5000
```

**Features:**
- ✅ Visual parameter configuration (sliders, dropdowns)
- ✅ Real-time console output and progress tracking
- ✅ Results viewer with CSV download
- ✅ No command-line knowledge required
- ✅ Modern purple/violet gradient UI

👉 See [Web Dashboard Guide](#-web-dashboard-guide) for detailed usage

---

## 📖 CLI Tool Guide (run_all.py)

The `run_all.py` script is the main automation tool for running NS-3 simulations. It handles NS-3 builds, runs multiple protocols/configurations, and organizes results.

### Basic Syntax

```bash
python3 scripts/run_all.py [OPTIONS]
```

### Quick Examples

```bash
# Test GAODV with paper defaults (50 nodes, 500s, 5 runs)
python3 scripts/run_all.py --protocols GAODV --runs 5

# Compare GAODV vs AODV
python3 scripts/run_all.py --protocols GAODV AODV --runs 10

# Test scalability (25, 50, 100 nodes)
python3 scripts/run_all.py --protocols GAODV --nodes 25 50 100 --runs 5

# All protocols comparison
python3 scripts/run_all.py --protocols GAODV AODV OLSR DSDV DSR --runs 10

# Custom scenario (dense network, high traffic)
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --nodes 100 \
    --node-spacing 30 \
    --txrange 100 \
    --packet-rate 8 \
    --simtime 300 \
    --runs 5
```

### Output and Results

**Results Location:**
```
results/results_<PROTOCOL>_session_<TIMESTAMP>.csv
```

**CSV Columns:**
```
Protocol, Nodes, Sinks, SimTime, PacketSize, PacketRate, 
TXRange, NodeSpacing, InitialEnergy, 
PDR, PacketsSent, PacketsReceived, Throughput, EnergyConsumed
```

**Example Output:**
```
Running: GAODV | Nodes: 50 | Run: 1/5
Executing simulation...
✓ PDR: 99.98%
✓ Throughput: 20.04 kbps
✓ Energy: 1045.2 J
✓ Packets: 19571/19575

Results saved to: results/results_GAODV_session_20260208_143022.csv
```

### Advanced Usage

**Batch Testing:**
```bash
# Test multiple parameter combinations
for spacing in 30 50 75 100; do
    python3 scripts/run_all.py \
        --protocols GAODV AODV \
        --node-spacing $spacing \
        --txrange $((spacing * 2)) \
        --runs 10
done
```

**Skip Build (after first run):**
```bash
# First run: builds NS-3
python3 scripts/run_all.py --protocols GAODV --runs 5

# Subsequent runs: skip build for speed
python3 scripts/run_all.py --protocols AODV OLSR --runs 5 --skip-build
```

**Custom NS-3 Path:**
```bash
python3 scripts/run_all.py \
    --ns3-path /home/user/workspace/ns-allinone-3.42/ns-3.42 \
    --protocols GAODV \
    --runs 5
```

---

## 🌐 Web Dashboard Guide

A modern web-based dashboard for running simulations without using the command line.

### Starting the Dashboard

```bash
# Method 1: One-click launcher (recommended)
python3 dashboard/start.py

# Method 2: Manual start
cd dashboard
python3 server.py
```

The dashboard will:
1. ✓ Start server on port 5000
2. ✓ Automatically open http://localhost:5000 in your browser
3. ✓ Display the GAODV simulation interface

### Dashboard Features

#### 1. **Parameter Configuration Panel**

Configure all simulation parameters visually:

| Section | Parameters | Description |
|---------|------------|-------------|
| **Protocols** | Checkboxes for GAODV, AODV, OLSR, DSDV, DSR | Select which protocols to compare |
| **Network** | Nodes, Sinks, Node Spacing | Network topology settings |
| **Simulation** | Simulation Time, Runs | Duration and repetitions |
| **Traffic** | Packet Size, Packet Rate | UDP traffic generation |
| **Wireless** | TX Range, TX Power | Radio configuration |
| **Energy** | Initial Energy, Currents (Idle, RX, TX) | Battery model parameters |
| **GAODV** | ETHF (Energy Threshold Factor) | Protocol-specific settings |

#### 2. **Command Display** (NEW in v0.4)

Shows the exact `run_all.py` command being executed:

```
🚀 Executing run_all.py Command
────────────────────────────────────────
python3 scripts/run_all.py --protocols GAODV AODV --nodes 50 --runs 5 ...
────────────────────────────────────────
```

**Benefits:**
- ✅ See exact command for reproducibility
- ✅ Copy command for CLI use
- ✅ Verify parameters before execution

#### 3. **Real-Time Progress**

Visual progress tracking during simulation:

- **Progress Bar**: 0% → 100% with percentage
- **Current Protocol**: Shows which protocol is running
- **Run Counter**: "Run 3/5" progress indicator
- **Live Console**: Real-time NS-3 output

#### 4. **Results Viewer**

Browse and download simulation results:

- **CSV File List**: All results from `results/` folder
- **Download Button**: Download individual CSV files
- **Auto-Refresh**: Updates when new results are available
- **Timestamp Sorting**: Most recent results first

### Dashboard Usage Workflow

1. **Configure Parameters**: Use sliders/inputs to set simulation parameters
2. **Review Command**: Check the generated command in command display
3. **Start Simulation**: Click "Start Simulation" button
4. **Monitor Progress**: Watch real-time console output and progress bar
5. **View Results**: Download CSV files from results viewer

### Dashboard vs CLI

| Feature | Dashboard | CLI |
|---------|-----------|-----|
| **Ease of Use** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Parameter Discovery** | ⭐⭐⭐⭐⭐ (visual) | ⭐⭐ (need docs) |
| **Batch Testing** | ⭐⭐ (manual) | ⭐⭐⭐⭐⭐ (scripting) |
| **Real-time Feedback** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Automation** | ⭐ | ⭐⭐⭐⭐⭐ |
| **Reproducibility** | ⭐⭐⭐⭐ (command) | ⭐⭐⭐⭐⭐ (native) |

> **Recommendation:** Use Dashboard for exploration and learning, CLI for production research and automation.

---

## 🎛️ Parameter Reference

Complete reference for all simulation parameters.

### Protocol Selection

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| `--protocols` | `GAODV` `AODV` `OLSR` `DSDV` `DSR` | `GAODV` | Routing protocols to test |

**Example:**
```bash
# Single protocol
python3 scripts/run_all.py --protocols GAODV

# Multiple protocols (comparison)
python3 scripts/run_all.py --protocols GAODV AODV OLSR
```

---

### Network Topology

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `--nodes` | `50` | 10-500 | Number of nodes in network |
| `--sinks` | `10` | 1-50 | Number of sink/receiver nodes |
| `--node-spacing` | `50` | 10-200 | Distance between nodes (meters) |

**Grid Layout:**
- Nodes arranged in square grid: $\sqrt{N} \times \sqrt{N}$
- For 50 nodes: ~7×7 grid
- Total area: $(spacing \times \sqrt{N})^2$
- Example: 50 nodes with 50m spacing = ~350m × 350m

**Multiple Values:**
```bash
# Test different network sizes
python3 scripts/run_all.py --nodes 25 50 100 --runs 5
```

---

### Simulation Duration

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `--simtime` | `500` | 50-10000 | Simulation time (seconds) |
| `--runs` | `5` | 1-100 | Runs per configuration |

**Battery Considerations:**
- Default energy (20J) lasts ~222 seconds
- For simtime > 222s, increase `--initial-energy`

```bash
# 500s simulation (requires more energy)
python3 scripts/run_all.py --simtime 500 --initial-energy 50
```

---

### Traffic Generation

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `--packet-size` | `512` | 64-1500 | UDP packet size (bytes) |
| `--packet-rate` | `4` | 1-50 | Packets per second per sink |

**Traffic Load Calculation:**
```
Total packets/sec = packet_rate × num_sinks
Total data rate = packet_size × packet_rate × num_sinks × 8 (bits/sec)

Example (defaults):
  = 512 bytes × 4 pkt/s × 10 sinks × 8 bits/byte
  = 163,840 bits/sec = ~164 kbps network load
```

**Examples:**
```bash
# Light traffic (low load)
python3 scripts/run_all.py --packet-rate 2 --packet-size 256

# Heavy traffic (stress test)
python3 scripts/run_all.py --packet-rate 10 --packet-size 1024
```

---

### Wireless/RF Configuration

| Parameter | Default | Range | Units | Description |
|-----------|---------|-------|-------|-------------|
| `--txrange` | `150` | 50-500 | meters | Transmission range (WiFi coverage) |
| `--txpower` | `7.5` | 0-20 | dBm | Transmission power |

**Critical Rule:**
```
TX Range > Node Spacing (required for connectivity)
```

**TX Power vs Range:**
- Uses **Friis propagation model** in free space
- Relationship: $Range \propto \sqrt{Power}$
- Higher power = Longer range + More energy consumption

**Examples:**
```bash
# Dense network (close nodes)
python3 scripts/run_all.py --node-spacing 30 --txrange 100

# Sparse network (far nodes)
python3 scripts/run_all.py --node-spacing 100 --txrange 200

# Test TX power effect
python3 scripts/run_all.py --txpower 5.0 7.5 10.0
```

---

### Energy Model

| Parameter | Default | Units | Description |
|-----------|---------|-------|-------------|
| `--initial-energy` | `20` | Joules | Initial battery capacity |
| `--idle-current` | `0.0273` | Amps | Idle state current (27.3mA) |
| `--rx-current` | `0.0313` | Amps | RX state current (31.3mA) |
| `--tx-current-base` | `0.010` | Amps | Base TX current (10mA + power-dependent) |

**Energy Model:** Uses NS-3 `WifiRadioEnergyModel` with accurate PHY state tracking.

**Battery Life Formula:**
```
Lifetime (sec) = Initial Energy (J) / (Idle Current (A) × Voltage (V))

Default:
  = 20J / (0.0273A × 3.3V)
  = 20 / 0.09009
  = 222 seconds

For 500s simulation:
  Required energy = (0.0273A × 3.3V) × 500s = 45J
  Use: --initial-energy 50
```

**Power Consumption by State:**

| State | Current | Power @ 3.3V | Notes |
|-------|---------|--------------|-------|
| TX | 380mA | 1.25W | Highest consumption |
| RX | 313mA | 1.03W | Listening for packets |
| Idle | 273mA | 0.90W | Radio on, not TX/RX |
| Sleep | 33mA | 0.11W | Deep sleep (not used) |

**Examples:**
```bash
# Long simulation (increase battery)
python3 scripts/run_all.py --simtime 1000 --initial-energy 100

# Low-power nodes (reduce currents)
python3 scripts/run_all.py --idle-current 0.01 --rx-current 0.01
```

---

### GAODV-Specific Parameters

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `--ethf` | `0.2` | 0.0-1.0 | Energy Threshold Factor |
| `--alpha` | `0.6` | 0.4-0.8 | Alpha parameter (hardcoded) |

**Energy Threshold Factor (ETHF):**
- Nodes with $NEF < ETHF$ drop RREQ messages
- Preserves energy for critical nodes
- Default 0.2 = drop RREQ when battery < 20%

**Node Energy Factor (NEF):**
```
NEF = Residual Energy / Initial Energy
```

**Example:**
```bash
# Conservative (protect low-battery nodes early)
python3 scripts/run_all.py --ethf 0.3

# Aggressive (use all available energy)
python3 scripts/run_all.py --ethf 0.1
```

---

### System Options

| Parameter | Description | Example |
|-----------|-------------|---------|
| `--ns3-path` | NS-3 installation directory | `--ns3-path /home/user/ns-3.42` |
| `--skip-build` | Skip NS-3 rebuild (faster) | `--skip-build` |
| `--no-collect` | Don't move results to results/ | `--no-collect` |

**Build Optimization:**
```bash
# First run: build NS-3
python3 scripts/run_all.py --protocols GAODV --runs 5

# Subsequent runs: skip build (3x faster)
python3 scripts/run_all.py --protocols AODV --runs 5 --skip-build
python3 scripts/run_all.py --protocols OLSR --runs 5 --skip-build
```

---

## 📊 Performance Metrics

Understanding simulation output and results.

### Primary Metrics

| Metric | Description | Formula | Good Values |
|--------|-------------|---------|-------------|
| **PDR (%)** | Packet Delivery Ratio | $(Received / Sent) \times 100$ | > 95% |
| **Throughput (kbps)** | Data rate to receivers | $(Received \times PacketSize \times 8) / SimTime / 1000$ | Depends on load |
| **Energy (J)** | Total consumed energy | $\sum$(TX + RX + Idle energy) | Lower is better |
| **Packets Sent** | Total packets transmitted | Count from UDP sources | - |
| **Packets Received** | Total packets delivered | Count at sinks | - |

### Typical Results (Paper Configuration)

**Configuration:** 50 nodes, 10 sinks, 500s, 512B packets, 4 pkt/s

| Protocol | PDR | Throughput | Energy | Notes |
|----------|-----|------------|--------|-------|
| **GAODV** | 99.98% | 20.04 kbps | ~1000J | Energy-aware routing |
| **AODV** | 100% | 20.04 kbps | ~1000J | Standard reactive |
| **OLSR** | ~98% | ~20 kbps | ~1000J | Proactive routing |
| **DSDV** | ~96% | ~19.6 kbps | ~1000J | Table-driven |
| **DSR** | ~99% | ~20 kbps | ~1000J | Source routing |

### Energy Tracking Details

**WifiRadioEnergyModel** (NS-3 built-in):
- Connects to WiFi PHY state machine
- Tracks actual TX/RX/Idle/Sleep state transitions
- Power-dependent TX current: more power = more energy
- Provides accurate per-activity consumption

**Energy Flow:**
```
Initial Energy (20J)
    ↓
TX Energy (packets transmitted)
    + RX Energy (packets received)
    + Idle Energy (radio on, waiting)
    ↓
Residual Energy (remaining)
```

**CSV Output Example:**
```csv
Protocol,Nodes,Sinks,SimTime,PacketSize,PacketRate,TXRange,NodeSpacing,InitialEnergy,PDR,PacketsSent,PacketsReceived,Throughput,EnergyConsumed
GAODV,50,10,500,512,4,150,50,20,99.98,19575,19571,20.04,1045.2
```

### Interpreting Results

**High PDR (> 99%):**
- ✅ Good network connectivity
- ✅ TX range sufficient for topology
- ✅ Routing protocol working correctly

**Low PDR (< 90%):**
- ❌ TX range too short for node spacing
- ❌ Network partitioning
- ❌ High mobility (not applicable in static grid)
- **Fix:** Increase `--txrange` or reduce `--node-spacing`

**High Energy Consumption:**
- Expected for large networks or long simulations
- Increase `--initial-energy` if nodes deplete before sim end
- Higher `--txpower` increases energy consumption

---

## 📐 EDA-AODV Paper Configuration

Reference parameters from the original EDA-AODV research paper (Table 1).

### Paper Parameters

| Parameter | Paper Value | CLI Option | Notes |
|-----------|-------------|------------|-------|
| Number of Nodes | 50 | `--nodes 50` | Default |
| Simulation Time | 500s | `--simtime 500` | Default |
| Initial Energy | 20 J | `--initial-energy 20` | Default |
| Packet Size | 512 B | `--packet-size 512` | Default |
| Packet Rate | 4 pkt/s | `--packet-rate 4` | Default |
| Energy Threshold (ETHF) | 20% | `--ethf 0.2` | Default |
| Alpha (α) | 0.6 | (hardcoded) | Range: 0.4-0.8 |
| MAC Layer | 802.11a | (hardcoded) | WiFi standard |
| TX Range | 150 m | `--txrange 150` | Default |
| Node Spacing | 50 m | `--node-spacing 50` | Default |

### Reproduce Paper Results

```bash
# Exact paper configuration (all defaults)
python3 scripts/run_all.py --protocols GAODV --runs 10

# Or explicitly specify all parameters
python3 scripts/run_all.py \
    --protocols GAODV \
    --nodes 50 \
    --sinks 10 \
    --simtime 500 \
    --initial-energy 20 \
    --packet-size 512 \
    --packet-rate 4 \
    --ethf 0.2 \
    --txrange 150 \
    --node-spacing 50 \
    --runs 10
```

### Expected Paper Results

Based on EDA-AODV paper findings:

**Energy Performance:**
- GAODV extends network lifetime vs standard AODV
- More balanced energy consumption across nodes
- Avoids low-energy nodes in routing paths

**Routing Performance:**
- PDR comparable to AODV (> 99%)
- Slightly higher latency due to energy considerations
- Better resilience to node failures

**Comparison:**
| Metric | GAODV | AODV | Improvement |
|--------|-------|------|-------------|
| Network Lifetime | Longer | Baseline | +15-30% |
| Energy Balance | Better | Uneven | Energy-aware paths |
| PDR | ~99.98% | ~100% | Comparable |

---

## 🧪 Validation Suite

Automated tests to verify GAODV implementation correctness.

### Quick Validation

Run complete test suite (takes ~5 minutes):

```bash
# Make scripts executable
chmod +x scripts/validate_*.sh scripts/full_gaodv_validation.sh

# Run all tests
./scripts/full_gaodv_validation.sh
```

### Expected Output

```
╔═══════════════════════════════════════════════════════════════╗
║    GAODV PROTOCOL - COMPLETE VALIDATION SUITE                 ║
╚═══════════════════════════════════════════════════════════════╝

[1/5] Testing Energy Consumption Variability... ✅ PASS
      TX power affects energy consumption correctly

[2/5] Testing Energy Source Connection...       ✅ PASS
      NEF calculations use real energy values
      NEF range: 0.80-0.95 (expected: not always 1.0)

[3/5] Testing MaxRange Parameter...             ✅ PASS
      Distance filtering active in routing decisions
      Routes filtered by MaxRange threshold

[4/5] Testing RREQEF Flowchart Logic...         ✅ PASS
      Position (X,Y) updates correctly
      RREQEF updates when NEF < RREQEF
      Minimum energy path selection works

[5/5] Command Display Test                      ℹ️  Manual
      Check: Command appears once at start
      Location: Terminal output or dashboard

╔═══════════════════════════════════════════════════════════════╗
║    ✅ VALIDATION COMPLETE - All automated tests passed         ║
╚═══════════════════════════════════════════════════════════════╝
```

### Individual Test Scripts

#### Test 1: Energy Source Connection

```bash
./scripts/validate_energy_connection.sh
```

**Validates:**
- ✅ NEF calculations access `EnergySourceContainer`
- ✅ NEF values reflect real energy depletion (0.8-0.95)
- ✅ NEF not stuck at 1.0 (bug from v0.3)

**Bug Fixed:** v0.4 fixed `ComputeNef()` to properly read node energy.

#### Test 2: MaxRange Parameter Enforcement

```bash
./scripts/validate_maxrange.sh
```

**Validates:**
- ✅ MaxRange actively filters RREQ packets by distance
- ✅ Routes beyond MaxRange are rejected
- ✅ Distance calculations use node positions

**Bug Fixed:** v0.4 activated MaxRange filtering (was defined but unused).

#### Test 3: RREQEF Flowchart Logic

```bash
./scripts/validate_rreqef_logic.sh
```

**Validates:**
- ✅ Position (X,Y) updated at each hop
- ✅ RREQEF updates when `NEF < RREQEF`
- ✅ Destination selects path with max RREQEF
- ✅ All 11 EDA-AODV flowchart steps implemented

**Flowchart Compliance:** 100% alignment with EDA-AODV paper Figure 2.

#### Test 4: Legacy Verification

```bash
./verify_implementation.sh
```

**Validates:**
- ✅ C++ source files present
- ✅ Python scripts executable
- ✅ Dependencies installed

**Note:** This is the original verification script from v0.2-v0.3.

### Validation Results Summary

All validation tests confirm:

1. **Energy Model Working** - NEF calculations use real battery levels
2. **Distance Filtering Active** - MaxRange enforced correctly
3. **EDA-AODV Compliance** - 100% flowchart implementation
4. **No Regressions** - All bug fixes verified

---

## 📝 Example Test Cases

Ready-to-run commands for common research scenarios.

### Test 1: Quick Validation (Small Network)

```bash
# 10 nodes, 100s simulation - fast validation
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --nodes 10 \
    --simtime 100 \
    --runs 1
```

**Duration:** ~2 minutes  
**Use Case:** Quick functionality check

---

### Test 2: Paper Default Configuration

```bash
# EDA-AODV paper parameters: 50 nodes, 500s, 20J energy
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --nodes 50 \
    --simtime 500 \
    --runs 5
```

**Duration:** ~30 minutes  
**Use Case:** Reproduce paper results

---

### Test 3: Protocol Comparison (All Protocols)

```bash
# Compare GAODV with all standard protocols
python3 scripts/run_all.py \
    --protocols GAODV AODV OLSR DSDV DSR \
    --nodes 50 \
    --runs 3
```

**Duration:** ~45 minutes  
**Use Case:** Comprehensive protocol evaluation

---

### Test 4: Network Scalability

```bash
# Test scalability: 25, 50, 100 nodes
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --nodes 25 50 100 \
    --runs 5
```

**Duration:** ~60 minutes  
**Use Case:** Scalability analysis for research paper

---

### Test 5: TX Range vs Node Spacing

```bash
# Test 1: Default (good connectivity)
python3 scripts/run_all.py --protocols GAODV AODV --runs 3

# Test 2: Larger spacing (needs longer range)
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --node-spacing 100 \
    --txrange 150 \
    --runs 3

# Test 3: Larger spacing with higher TX power
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --node-spacing 100 \
    --txrange 200 \
    --runs 3

# Test 4: Dense network (closer nodes)
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --node-spacing 30 \
    --txrange 100 \
    --runs 3
```

**Use Case:** Topology parameter impact study

---

### Test 6: High Traffic Load

```bash
# 8 packets/second with 1024B packets
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --packet-rate 8 \
    --packet-size 1024 \
    --runs 5
```

**Traffic Load:** ~655 kbps  
**Use Case:** Stress testing

---

### Test 7: Extended Duration (Energy Depletion)

```bash
# 1000 second simulation for energy analysis
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --simtime 1000 \
    --initial-energy 100 \
    --runs 3
```

**Use Case:** Energy depletion patterns, network lifetime

---

### Test 8: ETHF Sensitivity Analysis

```bash
# Test different energy thresholds
python3 scripts/run_all.py \
    --protocols GAODV \
    --ethf 0.1 0.2 0.3 \
    --runs 5
```

**Use Case:** Energy threshold parameter tuning

---

### Test 9: Full Benchmark Suite (Research Paper)

```bash
# Complete benchmark for publication
python3 scripts/run_all.py \
    --protocols GAODV AODV OLSR DSDV \
    --nodes 25 50 100 \
    --runs 10

# Generate visualizations
python3 scripts/plot_results.py
```

**Duration:** ~3-4 hours  
**Use Case:** Complete research dataset with graphs

---

## 🧮 GAODV Algorithm

Technical details of the GAODV routing algorithm based on EDA-AODV.

### Algorithm Flowchart

GAODV follows this 11-step flowchart:

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

#### 1. Node Energy Factor (NEF)

```
NEF = E_residual / E_initial
```

- `E_residual`: Current remaining energy (Joules)
- `E_initial`: Initial battery capacity (20J default)
- Range: 0.0 to 1.0

**Example:**
```
Node starts: 20J → NEF = 1.0
After 250s:  16J → NEF = 0.8
After 400s:  10J → NEF = 0.5
```

#### 2. Energy Threshold Check

```
Condition: NEF >= ETHF

If NEF < ETHF:
    Drop RREQ (preserve remaining energy)
```

- `ETHF`: Energy Threshold Factor (0.2 default = 20%)
- Nodes below threshold refuse to participate in route discovery

**Example:**
```
ETHF = 0.2 (20%)
Node with NEF = 0.25 → ACCEPTS RREQ (25% > 20%)
Node with NEF = 0.15 → DROPS RREQ   (15% < 20%)
```

#### 3. RREQ Energy Factor Update

```
At each intermediate node:

IF NEF_current < RREQEF_packet:
    RREQEF_new = NEF_current  (update to lower value)
    Position_X = current_node.x
    Position_Y = current_node.y
ELSE:
    RREQEF_new = RREQEF_packet (keep existing)
    Position_X = current_node.x  (still update position!)
    Position_Y = current_node.y
```

**RREQEF tracks the minimum energy along the path.**

**Example Path:**
```
Source → Node A (NEF=0.9) → Node B (NEF=0.7) → Node C (NEF=0.85) → Dest

At Node A: RREQEF = min(1.0, 0.9) = 0.9
At Node B: RREQEF = min(0.9, 0.7) = 0.7  (bottleneck node!)
At Node C: RREQEF = min(0.7, 0.85) = 0.7 (unchanged)
Destination receives: RREQEF = 0.7 (weakest node has 70% energy)
```

#### 4. Position Update Logic

| Condition | Position (X,Y) | RREQEF |
|-----------|----------------|--------|
| NEF < RREQEF | ✅ Updated to current node | ✅ Updated to NEF |
| NEF >= RREQEF | ✅ Updated to current node | ⏹️ Unchanged |

**Position is ALWAYS updated before rebroadcast.**

#### 5. Path Selection at Destination

```
Best_Path = argmax(RREQEF_path)
             all_paths
```

**Destination selects the path with MAXIMUM RREQEF.**

**Example (Destination receives 3 paths):**
```
Path 1: RREQEF = 0.6 (weakest node: 60% energy)
Path 2: RREQEF = 0.8 (weakest node: 80% energy) ← SELECTED
Path 3: RREQEF = 0.7 (weakest node: 70% energy)

Selected: Path 2 (best bottleneck node has most energy)
```

### Algorithm Properties

**Energy-Aware:**
- Routes avoid low-energy nodes
- Balances energy consumption across network
- Extends overall network lifetime

**Distance-Aware:**
- MaxRange parameter filters distant nodes
- Prevents long-hop routes (not in original AODV)
- Maintains connectivity in sparse networks

**Adaptive:**
- NEF recalculated at each hop
- Path selection considers current energy state
- Responds to energy depletion patterns

---

## 📁 Project Structure

```
AODV-FreeSpace-Research/
│
├── README.md                             # This comprehensive user guide
├── requirements.txt                      # Python dependencies (matplotlib, pandas)
├── IMPLEMENTATION_SUMMARY.md             # v0.4 development session log
├── verify_implementation.sh              # Legacy verification script
│
├── docs/                                 # 📚 Documentation
│   ├── README.md                         # Documentation index
│   ├── RELEASE_v0.4.md                   # ⭐ Complete v0.4 reference (1100+ lines)
│   ├── batch-v0.3-fix.md                 # v0.3 energy model changes
│   └── batch_v0.2_fix.md                 # v0.2 PDR bug fixes
│
├── src/                                  # 🎯 NS-3 Simulation Scenario
│   └── manet-compare-freespace-v1.cc     # Main simulation (goes to ns-3/scratch/)
│
├── scripts/                              # 🤖 Automation & Validation
│   ├── run_all.py                        # ⭐ Main CLI automation tool
│   ├── plot_results.py                   # Results visualization (matplotlib)
│   ├── README.md                         # Scripts documentation
│   │
│   ├── validate_energy_connection.sh     # Test 1: NEF calculations
│   ├── validate_maxrange.sh              # Test 2: Distance filtering
│   ├── validate_rreqef_logic.sh          # Test 3: RREQEF flowchart
│   └── full_gaodv_validation.sh          # Complete test suite
│
├── dashboard/                            # 🌐 Web Dashboard (v0.4)
│   ├── start.py                          # One-click launcher
│   ├── server.py                         # Flask backend server
│   ├── public/
│   │   └── index.html                    # Single-page app UI
│   └── api/
│       ├── simulation.py                 # Simulation runner (calls run_all.py)
│       ├── parameter_validator.py        # Parameter validation
│       └── visualizer.py                 # Data visualization
│
├── gaodv-v1/                             # 🔧 GAODV Protocol Module (ns-3/src/)
│   ├── CMakeLists.txt                    # Build configuration
│   ├── gaodv-v1-helper.cc/h              # NS-3 helper class
│   ├── gaodv-v1-routing-protocol.cc/h    # ⭐ Core routing logic (NEF, RREQEF)
│   ├── gaodv-v1-packet.cc/h              # RREQ/RREP/RERR packets with RREQEF
│   ├── gaodv-v1-rtable.cc/h              # Routing table with energy info
│   ├── gaodv-v1-neighbors.cc/h           # Neighbor management
│   ├── gaodv-v1-rqueue.cc/h              # Request queue
│   ├── gaodv-v1-id-cache.cc/h            # Duplicate detection
│   └── gaodv-v1-dpd.cc/h                 # Duplicate packet detection
│
├── aodv/                                 # 📦 Standard AODV (ns-3 contrib)
│   ├── aodv-routing-protocol.cc/h        # AODV implementation
│   ├── aodv-packet.cc/h                  # AODV packet types
│   ├── aodv-rtable.cc/h                  # Routing table
│   └── ...                               # (other AODV files)
│
├── dsdv/                                 # 📦 DSDV Protocol
│   ├── dsdv-routing-protocol.cc/h        # Table-driven routing
│   └── ...
│
├── dsr/                                  # 📦 DSR Protocol
│   ├── dsr-routing.cc/h                  # Source routing
│   └── ...
│
├── results/                              # 📊 Simulation Output
│   ├── results_GAODV_session_*.csv       # GAODV results
│   ├── results_AODV_session_*.csv        # AODV results
│   └── ...                               # (other protocol results)
│
└── test/                                 # 🧪 NS-3 Regression Tests
    ├── aodv-chain-regression-test-*.pcap # PCAP captures for validation
    └── ...
```

### Key Files Explained

#### Core Simulation
- **[src/manet-compare-freespace-v1.cc](src/manet-compare-freespace-v1.cc)** (640 lines)
  - Main NS-3 simulation scenario
  - Configures grid topology, WiFi, energy model
  - Runs comparison of GAODV vs AODV/OLSR/DSDV/DSR
  - Outputs metrics (PDR, throughput, energy)

#### GAODV Protocol
- **[gaodv-v1-routing-protocol.cc](gaodv-v1/gaodv-v1-routing-protocol.cc)** (~2000 lines)
  - Core GAODV algorithm implementation
  - Functions: `ComputeNef()`, `RecvRequest()`, `SendReply()`
  - RREQEF update logic (Step 6 of flowchart)
  - MaxRange distance filtering
  - Energy threshold checking

- **[gaodv-v1-packet.cc](gaodv-v1/gaodv-v1-packet.cc)**
  - Packet structures: RREQ, RREP, RERR
  - RREQEF field in RREQ header
  - Position (X,Y) fields for distance calculation

#### Automation
- **[scripts/run_all.py](scripts/run_all.py)** (500+ lines)
  - Main automation script
  - Handles NS-3 builds, multiple runs, result collection
  - All CLI parameters defined here

#### Dashboard
- **[dashboard/api/simulation.py](dashboard/api/simulation.py)** (400+ lines)
  - Backend simulation runner
  - Calls `run_all.py` with parameters
  - Real-time progress tracking
  - **v0.4 Fix:** Single run_all.py call (not per-protocol)

---

## 💻 Direct NS-3 Usage

For advanced debugging or custom scenarios, you can run NS-3 directly without `run_all.py`.

### Basic Usage

```bash
cd /path/to/ns-3.42

# Default parameters (50 nodes, 500s, GAODV)
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# Quick test (10 nodes, 100s, AODV)
./ns3 run "scratch/manet-compare-freespace-v1 \
    --protocol=AODV \
    --nWifis=10 \
    --totalTime=100"

# Custom configuration
./ns3 run "scratch/manet-compare-freespace-v1 \
    --protocol=GAODV \
    --nWifis=50 \
    --nSinks=10 \
    --totalTime=500 \
    --txRange=150 \
    --packetRate=4 \
    --packetSize=512"
```

### All NS-3 Parameters

```bash
# Show help with all available options
./ns3 run "scratch/manet-compare-freespace-v1 --PrintHelp"
```

**Available Options:**
```
--protocol=GAODV          Protocol: GAODV, AODV, OLSR, DSDV, DSR
--nWifis=50               Number of nodes
--nSinks=10               Number of sinks
--totalTime=500           Simulation time (seconds)
--txRange=150             TX range (meters)
--nodeSpacing=50          Node spacing in grid (meters)
--packetSize=512          Packet size (bytes)
--packetRate=4            Packets per second
--initialEnergy=20        Initial energy (Joules)
--ethf=0.2                Energy threshold factor (GAODV)
--txPower=7.5             TX power (dBm)
--idleCurrent=0.0273      Idle current (Amps)
--rxCurrent=0.0313        RX current (Amps)
--txCurrentBase=0.010     Base TX current (Amps)
```

### Debugging Techniques

**Enable NS-3 Logging:**
```bash
# GAODV routing protocol logs
export NS_LOG="GaodvV1RoutingProtocol=level_debug"
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# Multiple components
export NS_LOG="GaodvV1RoutingProtocol=level_debug:WifiRadioEnergyModel=level_info"
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# All components (very verbose!)
export NS_LOG="*=level_all"
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"
```

**PCAP Packet Capture:**
```bash
# Enable PCAP in manet-compare-freespace-v1.cc
# Uncomment: wifiPhy.EnablePcapAll("manet-compare");

./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# Analyze with Wireshark
wireshark manet-compare-0-0.pcap
```

**Animation (NetAnim):**
```bash
# Enable animation XML output in code
# Add: AnimationInterface anim("gaodv-animation.xml");

./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# View with NetAnim
netanim gaodv-animation.xml
```

### When to Use Direct NS-3

Use direct NS-3 commands when you need:

- **Detailed Logging**: Enable component-specific debug output
- **PCAP Captures**: Analyze packet-level behavior
- **Animation**: Visualize node movement and packet flow
- **Custom Modifications**: Test code changes before integrating with run_all.py
- **Single Runs**: Quick one-off tests without automation overhead

Use `run_all.py` for:

- **Batch Testing**: Multiple protocols/configurations
- **Result Collection**: Automatic CSV organization
- **Reproducible Research**: Consistent parameter sets
- **Integration**: Dashboard and visualization pipeline

---

## 📚 Documentation

Complete collection of project documentation.

### Release Notes

#### Primary Documentation

| Document | Description | Lines | Status |
|----------|-------------|-------|--------|
| **[RELEASE_v0.4.md](docs/RELEASE_v0.4.md)** | **Complete v0.4 reference guide** | 1100+ | ⭐ Current |
| [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) | Development session log | 340 | v0.4 |
| [README.md](README.md) | **This user guide** | 1400+ | ⭐ Current |

#### Version History

| Document | Version | Description |
|----------|---------|-------------|
| [docs/batch-v0.3-fix.md](docs/batch-v0.3-fix.md) | v0.3 | Energy model overhaul, critical bug fixes |
| [docs/batch_v0.2_fix.md](docs/batch_v0.2_fix.md) | v0.2 | PDR bug fixes, metrics implementation |

### What's in RELEASE_v0.4.md?

The [RELEASE_v0.4.md](docs/RELEASE_v0.4.md) document is the **comprehensive technical reference** with:

**Bug Fixes:**
- MaxRange parameter bug (definition vs usage)
- Energy Source Connection fix (NEF calculations)
- WiFi TX Power alignment (PHY vs energy model)

**New Features:**
- Web Dashboard architecture and usage
- Command display feature (terminal + dashboard)
- Validation suite (4 automated scripts)

**Flowchart Validation:**
- All 11 EDA-AODV steps verified
- Code-to-flowchart mapping
- RREQEF update logic validation

**Implementation Details:**
- Code changes with line numbers
- Before/after comparisons
- Technical architecture diagrams

**Migration Guide:**
- v0.3 → v0.4 upgrade instructions
- Breaking changes (none in v0.4)
- Compatibility notes

**Results & Benchmarks:**
- Performance data
- Energy tracking validation
- Test results from validation suite

### Documentation Index

See [docs/README.md](docs/README.md) for complete documentation navigation.

### Research Paper

**Original EDA-AODV Paper:**
[EDA-AODVEnergyandDistanceAwareAODVRoutingProtocol.pdf](EDA-AODVEnergyandDistanceAwareAODVRoutingProtocol.pdf)

**Key Sections:**
- Section 3: GAODV Algorithm Description
- Section 4.1: Path Selection (max RREQEF)
- Figure 2: GAODV Flowchart (11 steps)
- Table 1: Simulation Parameters

---

## 🔧 Troubleshooting

Common issues and solutions.

### Installation Issues

**Problem: NS-3 not found**
```
Error: NS-3 directory not found
```

**Solution:**
```bash
# Specify NS-3 path explicitly
python3 scripts/run_all.py \
    --ns3-path /home/user/ns-allinone-3.42/ns-3.42 \
    --protocols GAODV
```

**Problem: Build errors**
```
Error: cmake not found / build failed
```

**Solution:**
```bash
# Clean and rebuild NS-3
cd /path/to/ns-3.42
./ns3 clean
./ns3 configure
./ns3 build

# Verify GAODV module installed
ls src/gaodv-v1/
```

---

### Simulation Issues

**Problem: Zero PDR (0% packet delivery)**

**Causes:**
1. TX range < node spacing (no connectivity)
2. Energy depleted before simulation end
3. Module not built correctly

**Solutions:**
```bash
# 1. Increase TX range
python3 scripts/run_all.py --txrange 200

# 2. Increase initial energy
python3 scripts/run_all.py --initial-energy 50 --simtime 500

# 3. Rebuild NS-3
cd /path/to/ns-3.42
./ns3 clean
./ns3 build
```

**Problem: Low PDR (< 90%)**

**Diagnosis:**
```bash
# Check if range < spacing
python3 scripts/run_all.py --node-spacing 100 --txrange 150
# Rule: txrange must be > node-spacing
```

**Fix:**
```bash
# Increase range
python3 scripts/run_all.py --node-spacing 100 --txrange 200
```

---

### Energy Issues

**Problem: NEF always 1.0 (energy not depleting)**

**Fixed in v0.4** - Run validation:
```bash
./scripts/validate_energy_connection.sh
```

**Problem: Nodes running out of energy**

**Solution:**
```bash
# Increase battery capacity
python3 scripts/run_all.py --initial-energy 100 --simtime 500

# Or reduce currents (low-power mode)
python3 scripts/run_all.py \
    --idle-current 0.01 \
    --rx-current 0.01 \
    --initial-energy 20
```

---

### Dashboard Issues

**Problem: Dashboard won't start**
```
Error: Address already in use - port 5000
```

**Solution:**
```bash
# Kill existing process
lsof -ti:5000 | xargs kill -9

# Or use different port (edit dashboard/server.py)
# Change: app.run(port=5001)
```

**Problem: No console output in dashboard**

**Check:** Command displayed at top ?
- ✅ Yes: Simulation running correctly
- ❌ No: Check browser console (F12) for errors

**Problem: Dashboard calls NS-3 directly**

**Fixed in v0.4** - Run test:
```bash
./scripts/test_dashboard_fix.sh
# Should show: "✅ All 13 tests PASSED"
```

---

### Results Issues

**Problem: Results folder empty**

**Causes:**
1. Simulation crashed
2. `--no-collect` flag used
3. File permissions

**Solutions:**
```bash
# Check NS-3 build directory for CSV
ls /path/to/ns-3.42/*.csv

# Move manually if needed
mv /path/to/ns-3.42/results*.csv results/

# Fix permissions
chmod 755 results/
```

**Problem: Can't read CSV files**

**Solution:**
```bash
# View in terminal
column -t -s, results/results_GAODV_*.csv | less

# Or use Python
python3 scripts/plot_results.py

# Or open in LibreOffice/Excel
```

---

### Performance Issues

**Problem: Simulations too slow**

**Optimizations:**
```bash
# 1. Skip rebuild after first run
python3 scripts/run_all.py --skip-build

# 2. Reduce simulation time
python3 scripts/run_all.py --simtime 100

# 3. Fewer nodes
python3 scripts/run_all.py --nodes 25
```

**Problem: High CPU usage**

**Expected:** NS-3 simulations are CPU-intensive
- Each run uses 1 CPU core at 100%
- Multiple runs are sequential (not parallel)

---

### Research Scope

**What Works (Validated):**
- ✅ PDR (Packet Delivery Ratio) - highly accurate
- ✅ Throughput - validated against paper baselines
- ✅ Packet counts - precise sender/receiver tracking
- ✅ Energy consumption - `WifiRadioEnergyModel` with PHY state tracking
- ✅ NEF calculations - real battery depletion (fixed in v0.4)
- ✅ RREQEF updates - energy-aware path selection (validated)
- ✅ MaxRange filtering - distance-aware routing (activated in v0.4)

**Known Limitations:**
- Mobility: Currently static grid topology only
- Scalability: Tested up to 100 nodes
- Propagation: Free-space model only (Friis)

---

## 📝 Version History

### v0.4 (Current) - Production Ready Release
**Release Date:** February 8, 2026

#### Critical Bug Fixes (3)
1. **MaxRange Parameter Bug** ⚠️ CRITICAL
   - **Issue:** MaxRange defined but never used in routing decisions
   - **Fix:** Activated distance filtering at intermediate nodes and destination
   - **Impact:** Routes now properly filtered by distance threshold
   - **Files:** `gaodv-v1-routing-protocol.cc`

2. **Energy Source Connection** ⚠️ CRITICAL
   - **Issue:** `ComputeNef()` couldn't access `EnergySourceContainer`, NEF always 1.0
   - **Fix:** Proper energy source linkage, NEF now shows real values 0.8-0.95
   - **Impact:** Energy-aware routing now functional
   - **Files:** `gaodv-v1-routing-protocol.cc`, `manet-compare-freespace-v1.cc`

3. **WiFi TX Power Configuration** ⚠️ CRITICAL
   - **Issue:** PHY layer used NS-3 default ~16dBm instead of configured 7.5dBm
   - **Fix:** Aligned `YansWifiPhy::SetTxPowerStart/End` with energy model TX power
   - **Impact:** Energy consumption now matches configured power levels
   - **Files:** `manet-compare-freespace-v1.cc` (2 lines)

#### New Features
- ✨ **Web Dashboard** - Modern UI at `http://localhost:5000`
  - Visual parameter configuration
  - Real-time progress tracking
  - Results viewer with CSV download
  - Purple/violet gradient theme

- ✨ **Command Display** - Shows exact `run_all.py` command
  - Terminal: ASCII box around command
  - Dashboard: Dedicated command display section
  - Single display at start (not per-protocol)

- ✨ **Validation Suite** - Automated testing (4 scripts)
  - `validate_energy_connection.sh` - NEF calculations
  - `validate_maxrange.sh` - Distance filtering
  - `validate_rreqef_logic.sh` - RREQEF flowchart
  - `full_gaodv_validation.sh` - Complete suite

#### Flowchart Validation
- ✅ **100% EDA-AODV Compliance** - All 11 flowchart steps verified
- ✅ **RREQEF Updates Working** - Energy-aware path selection functional
- ✅ **Distance Filtering Active** - MaxRange enforced correctly

#### Documentation
- 📄 **RELEASE_v0.4.md** (1100+ lines) - Comprehensive consolidated guide
- 📄 **IMPLEMENTATION_SUMMARY.md** (340 lines) - Development session log
- 📄 **DASHBOARD_FIX_COMPLETE.md** - Dashboard integration documentation
- 📄 Consolidated 5 bug fix docs into RELEASE_v0.4.md

#### Dashboard Fix
- 🐛 **Fixed:** Dashboard called `run_all.py` multiple times (once per protocol)
- ✅ **Now:** Single `run_all.py` call with all protocols as arguments
- ✅ **Validated:** 13/13 automated tests passed

#### Files Changed
- Modified: 7 files (NS-3 + Python + Dashboard)
- Added: 4 validation scripts
- Total: ~400 lines added/modified

**Status:** ✅ Production Ready - All core functionality validated

---

### v0.3 - Critical Bug Fixes & Energy Model Overhaul
**Release Date:** January 2026

#### Critical Fixes
- 🐛 **InitialEnergy Mismatch** (500J → 20J) - Resolved 0% PDR
- 🐛 **ComputeRef() Not Called** - REF-based path selection now active
- 🐛 **Alpha Integration** - Alpha parameter integrated into NEF threshold
- ✅ **Energy Model:** Switched to `WifiRadioEnergyModel` (PHY state-based)
- ✅ Alpha range corrected: 0.4-0.8 (was 0.5-0.7)

#### Features
- ✅ Separate timestamped CSV per run (no aggregation)
- 📄 Added `docs/batch-v0.3-fix.md` comprehensive documentation

#### Metrics (50 nodes, 500s)
- PDR: 99.99%
- Throughput: 20 kbps
- Energy: 1698J (100 nodes/500s)

---

### v0.2.1 - Full Metrics Suite
**Release Date:** January 2026

#### Critical Fixes
- 🐛 **Resolved 0% PDR** - Fixed WiFi Energy Model conflict
- ✅ **Path Selection:** Max RREQEF (EDA-AODV paper Section 4.1)
- ✅ **Energy Tracking:** Custom implementation bypassing PHY conflicts
- ✅ Corrected threshold: `NEF >= ETHF` (not `NEF >= α*ETHF`)
- ✅ 2D distance calculation (removed Z-axis for ground MANETs)

#### Improvements
- ✅ WiFi config matching AODV regression test pattern
- ✅ Removed excessive debug output
- 📄 Added `docs/batch_v0.2_fix.md` documentation

#### Metrics (50 nodes, 500s)
- PDR: 99.98%
- Throughput: 20 kbps
- Energy: ~1000J

---

### v0.2 - Route Selection & CLI
**Release Date:** January 2026

- Route selection: max RREQEF (EDA-AODV paper)
- Python CLI with all paper parameters
- Propagation speed: 3×10⁸ m/s (radio)
- Documentation overhaul

---

### v0.1 - Initial Implementation
**Release Date:** January 2026

- Initial GAODV-V1 implementation
- Basic RREQ/RREP/RERR functionality
- Grid topology support

---

## 📄 License

MIT License

Copyright (c) 2026 AODV-FreeSpace-Research

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

## 🙏 Acknowledgments

- **NS-3 Project** - Network Simulator 3 ([nsnam.org](https://www.nsnam.org))
- **EDA-AODV Paper Authors** - Original algorithm design
- **AODV Contributors** - Reference implementation in NS-3

---

**Questions? Issues?** Check [Troubleshooting](#-troubleshooting) or file an issue on the repository.

**Happy Researching! 🚀**
