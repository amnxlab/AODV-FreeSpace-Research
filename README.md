# GAODV: Energy-Aware AODV Routing Protocol for Free-Space MANETs

![NS-3](https://img.shields.io/badge/NS--3-3.42-blue)
![Python](https://img.shields.io/badge/Python-3.8%2B-green)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Version](https://img.shields.io/badge/Version-0.2-orange)

Implementation of **GAODV (Geometric and Energy-Aware AODV)** based on the **EDA-AODV** paper for terrestrial free-space MANETs.

> **Reference Paper:** EDA-AODV: Energy and Distance Aware AODV Routing Protocol

---

## 📋 Table of Contents

1. [Quick Start](#-quick-start)
2. [Simulation Parameters (EDA-AODV Paper)](#-eda-aodv-paper-parameters)
3. [GAODV Algorithm & Equations](#-gaodv-algorithm--equations)
4. [Python Automation Tool](#-python-automation-tool)
5. [Project Structure](#-project-structure)
6. [Troubleshooting](#-troubleshooting)
7. [Version History](#-version-history)

---

## 🚀 Quick Start

### Step 1: Install Prerequisites

```bash
# Python dependencies
pip3 install -r requirements.txt

# Ensure NS-3 3.42 is installed
```

### Step 2: Copy Files to NS-3

```bash
# Copy GAODV module
cp -r gaodv-v1/ /path/to/ns-3.42/src/gaodv-v1/

# Copy simulation file
cp src/manet-compare-freespace-v1.cc /path/to/ns-3.42/scratch/

# Build NS-3
cd /path/to/ns-3.42 && ./ns3 build
```

### Step 3: Run Simulation

#### Option A: Direct NS-3 Command
```bash
cd /path/to/ns-3.42

# Default EDA-AODV paper parameters
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# Custom parameters
./ns3 run "scratch/manet-compare-freespace-v1 \
    --protocol=GAODV \
    --nWifis=50 \
    --totalTime=500 \
    --initialEnergy=20 \
    --packetSize=512"
```

#### Option B: Python Automation (Recommended)
```bash
# Single protocol test
python3 scripts/run_all.py --protocols GAODV --runs 5

# Compare GAODV vs AODV with paper parameters
python3 scripts/run_all.py --protocols GAODV AODV \
    --simtime 500 \
    --initial-energy 20 \
    --packet-size 512 \
    --packet-rate 4
```

---

## 📐 EDA-AODV Paper Parameters

Based on **Table 1** from the EDA-AODV paper:

| Parameter | Value | CLI Option |
|-----------|-------|------------|
| **Number of Nodes** | 50 | `--nWifis=50` |
| **Simulation Time** | 500 seconds | `--totalTime=500` |
| **Simulation Area** | 400m × 800m | (hardcoded) |
| **Initial Energy** | **20 Joules** | `--initialEnergy=20` |
| **Packet Size** | **512 bytes** | `--packetSize=512` |
| **Packet Rate** | **4 packets/second** | `--packetRate=4` |
| **Energy Threshold** | **10%** of initial | (code: Ethf=0.1) |
| **MAC Layer** | 802.11a | OfdmRate6Mbps |
| **Transmission Range** | 250m | (code) |
| **Propagation Model** | Two-Ray Ground | (code) |

---

## 📊 GAODV Algorithm & Equations

GAODV implements the **EDA-AODV** (Energy and Distance Aware AODV) routing protocol.

### Node Energy Factor (NEF)

Each node calculates its energy factor as the ratio of remaining to initial energy:

$$
\text{NEF} = \frac{E_{\text{residual}}}{E_{\text{initial}}}
$$

| Variable | Description | Default |
|----------|-------------|---------|
| $E_{\text{residual}}$ | Current remaining energy | - |
| $E_{\text{initial}}$ | Initial energy at start | **20 J** |

### Route Selection: Max-Min Energy

GAODV uses **max-min energy selection** at the destination:

**Step 1 - RREQ Propagation:**
Each intermediate node updates RqEF to carry the minimum energy along the path:

$$
\text{RqEF}^{\text{new}} = \min(\text{RqEF}^{\text{current}}, \text{NEF}_{\text{node}})
$$

**Step 2 - Destination Selection:**
After waiting time T, the destination selects the path with the **highest RqEF**:

$$
\text{Best Path} = \arg\max(\text{RqEF})
$$

> **Why Max-Min?** This selects the path where the weakest node (bottleneck) has the most energy, maximizing route lifetime.

### Energy Threshold Check

Nodes only forward RREQ if their energy exceeds the threshold:

$$
\text{NEF}_{\text{node}} \geq \text{ETH}_F \times \alpha
$$

| Parameter | Default | Description |
|-----------|---------|-------------|
| $\text{ETH}_F$ | **0.1** (10%) | Energy threshold factor |
| $\alpha$ | **0.6** | Threshold multiplier |

### Distance Calculation (3D)

Distance between nodes includes all three axes:

$$
d = \sqrt{(x_2 - x_1)^2 + (y_2 - y_1)^2 + (z_2 - z_1)^2}
$$

---

## 🐍 Python Automation Tool

The `run_all.py` script automates simulation runs with parameter control.

### Full CLI Reference

```bash
python3 scripts/run_all.py [OPTIONS]
```

#### Protocol & Network Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | `GAODV` | Protocols: GAODV, AODV, DSR, OLSR, DSDV |
| `--nodes` | `50` | Number of nodes (supports multiple: `25 50 100`) |
| `--txpower` | `7.5` | TX power in dBm |
| `--runs` | `5` | Number of runs per configuration |

#### EDA-AODV Paper Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--simtime` | **500** | Simulation time (seconds) |
| `--sinks` | **10** | Number of sink nodes |
| `--initial-energy` | **20** | Initial energy per node (Joules) |
| `--packet-size` | **512** | Packet size (bytes) |
| `--packet-rate` | **4** | Packets per second |

#### System Options

| Option | Default | Description |
|--------|---------|-------------|
| `--ns3-path` | Auto-detect | Path to NS-3 installation |
| `--skip-build` | False | Skip NS-3 rebuild |
| `--no-aggregate` | False | Skip result aggregation |

### Usage Examples

#### Example 1: Basic Test with Paper Defaults
```bash
python3 scripts/run_all.py --protocols GAODV --runs 5
```
Uses all EDA-AODV paper defaults automatically.

#### Example 2: GAODV vs AODV Comparison
```bash
python3 scripts/run_all.py --protocols GAODV AODV --runs 10
```

#### Example 3: Custom Energy Settings
```bash
python3 scripts/run_all.py --protocols GAODV \
    --initial-energy 15 \
    --simtime 300 \
    --runs 5
```

#### Example 4: Multi-Node Scalability Test
```bash
python3 scripts/run_all.py --protocols GAODV AODV \
    --nodes 25 50 100 \
    --runs 10
```

#### Example 5: Full Parameter Sweep
```bash
python3 scripts/run_all.py \
    --protocols GAODV AODV DSR \
    --nodes 25 50 100 \
    --txpower 5.0 7.5 10.0 \
    --initial-energy 20 \
    --runs 10
```

### Generate Visualizations

```bash
python3 scripts/plot_results.py
```

Produces:
- `graphs/pdr_comparison.png` - PDR by protocol and nodes
- `graphs/energy_comparison.png` - Energy consumption
- `graphs/pdr_energy_tradeoff.png` - Trade-off analysis

---

## 📁 Project Structure

```
AODV-FreeSpace-Research/
├── src/
│   └── manet-compare-freespace-v1.cc    # NS-3 simulation (main)
├── scripts/
│   ├── run_all.py                        # Python automation tool
│   └── plot_results.py                   # Visualization generator
├── gaodv-v1/                             # GAODV-V1 Protocol Module
│   ├── gaodv-v1-routing-protocol.cc/h    # Main routing logic
│   ├── gaodv-v1-packet.cc/h              # RREQ/RREP with (x,y,z) & RqEF
│   ├── gaodv-v1-rtable.cc/h              # Routing table
│   ├── gaodv-v1-rqueue.cc/h              # Packet queue
│   └── ...                               # Helper modules
├── results/                              # Output CSVs
├── graphs/                               # Generated plots
├── requirements.txt                      # Python dependencies
└── README.md                             # This file
```

---

## 🔧 Troubleshooting

### NS-3 Not Found
```bash
# Specify path manually
python3 scripts/run_all.py --ns3-path /home/user/ns-3.42 --protocols GAODV
```

### Build Errors
```bash
# Clean rebuild
cd /path/to/ns-3.42
./ns3 clean
./ns3 configure --enable-examples
./ns3 build
```

### Zero PDR
Common causes:
1. **Wrong WiFi standard** - Must use 802.11a with OfdmRate6Mbps
2. **Apps start too early** - Routes need time to establish (100s default)
3. **Short simulation** - Use at least 500s for meaningful results

### Missing Python Modules
```bash
pip3 install -r requirements.txt
```

---

## 📝 Version History

### v0.2 (February 2026)
- ✅ **Route Selection Fixed**: Changed from complex REF formula to **max RqEF** (EDA-AODV paper)
- ✅ **Python Parameters**: Added CLI options for all EDA-AODV paper values
- ✅ **Propagation Speed**: Fixed from 1500 m/s to **3×10⁸ m/s** (radio free-space)
- ✅ **Energy Threshold**: Updated to **10%** (was 20%)
- ✅ **Documentation**: Complete README overhaul

### v0.1 (January 2026)
- Initial GAODV-V1 implementation
- 802.11a WiFi configuration
- Basic Python automation

---

## 📚 References

1. **EDA-AODV Paper**: "EDA-AODV: Energy and Distance Aware AODV Routing Protocol"
2. **NS-3 Documentation**: https://www.nsnam.org/docs/
3. **AODV RFC 3561**: https://tools.ietf.org/html/rfc3561

---

## 📄 License

MIT License - See LICENSE file for details.
