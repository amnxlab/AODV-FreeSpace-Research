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
2. [Test Scenarios](#-test-scenarios)
3. [CLI Reference](#-cli-reference)
4. [EDA-AODV Parameters](#-eda-aodv-paper-parameters)
5. [GAODV Algorithm](#-gaodv-algorithm--equations)
6. [Project Structure](#-project-structure)
7. [Troubleshooting](#-troubleshooting)

---

## 🚀 Quick Start

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

## 🧪 Test Scenarios

### Scenario 1: Basic Protocol Comparison
Compare GAODV with standard AODV using paper defaults:
```bash
python3 scripts/run_all.py --protocols GAODV AODV --runs 10
```

### Scenario 2: Network Scalability
Test how protocols perform as network size increases:
```bash
python3 scripts/run_all.py --protocols GAODV AODV --nodes 25 50 100 --runs 5
```

### Scenario 3: Large Network (500 nodes)
Stress test with large network:
```bash
python3 scripts/run_all.py --protocols GAODV AODV --nodes 500 --runs 3
```

### Scenario 4: Energy Efficiency Comparison
Compare energy consumption across all routing protocols:
```bash
python3 scripts/run_all.py --protocols GAODV AODV DSR OLSR DSDV --runs 10
```

### Scenario 5: Low Energy Conditions
Test protocol behavior with limited initial energy:
```bash
python3 scripts/run_all.py --protocols GAODV AODV --initial-energy 10 --runs 5
```

### Scenario 6: High Traffic Load
Test with higher packet rate and larger packets:
```bash
python3 scripts/run_all.py --protocols GAODV AODV \
    --packet-rate 8 \
    --packet-size 1024 \
    --runs 5
```

### Scenario 7: Long Duration Simulation
Extended simulation to observe energy depletion:
```bash
python3 scripts/run_all.py --protocols GAODV AODV \
    --simtime 1000 \
    --initial-energy 15 \
    --runs 3
```

### Scenario 8: Complete Benchmark Suite
Full comparison for research paper:
```bash
python3 scripts/run_all.py \
    --protocols GAODV AODV DSR OLSR DSDV \
    --nodes 25 50 100 \
    --runs 10

# Generate visualization
python3 scripts/plot_results.py
```

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

### System Options

| Option | Description |
|--------|-------------|
| `--ns3-path PATH` | Specify NS-3 installation path |
| `--skip-build` | Skip NS-3 rebuild (faster) |
| `--no-aggregate` | Don't combine result files |

### Direct NS-3 Commands

For single runs or debugging:
```bash
cd /path/to/ns-3.42

# Default parameters
./ns3 run "scratch/manet-compare-freespace-v1 --protocol=GAODV"

# Custom parameters
./ns3 run "scratch/manet-compare-freespace-v1 \
    --protocol=GAODV \
    --nWifis=100 \
    --totalTime=500 \
    --initialEnergy=20"
```

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
| Energy Threshold | **10%** | (code: Ethf=0.1) |
| MAC Layer | 802.11a | (hardcoded) |
| TX Range | 250m | (hardcoded) |

---

## 📊 GAODV Algorithm & Equations

### Node Energy Factor (NEF)
$$
\text{NEF} = \frac{E_{\text{residual}}}{E_{\text{initial}}}
$$

### Route Selection: Max-Min Energy

**RREQ Propagation** (each node updates minimum):
$$
\text{RqEF}^{\text{new}} = \min(\text{RqEF}^{\text{current}}, \text{NEF}_{\text{node}})
$$

**Destination Selection** (picks highest RqEF):
$$
\text{Best Path} = \arg\max(\text{RqEF})
$$

> This selects the path where the weakest node has the most energy, maximizing route lifetime.

### Energy Threshold
$$
\text{NEF}_{\text{node}} \geq \text{ETH}_F \times \alpha
$$

Where: ETH_F = 0.1 (10%), α = 0.6

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
| Zero PDR | Ensure 802.11a is used, simtime ≥ 500s |
| Missing modules | `pip3 install -r requirements.txt` |

---

## 📝 Version History

### v0.2 (February 2026)
- ✅ Route selection: max RqEF (EDA-AODV paper)
- ✅ Python CLI with all paper parameters
- ✅ Propagation speed: 3×10⁸ m/s (radio)
- ✅ Documentation overhaul

### v0.1 (January 2026)
- Initial GAODV-V1 implementation

---

## 📄 License

MIT License
