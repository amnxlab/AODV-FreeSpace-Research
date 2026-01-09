# Automation Scripts

This directory contains Python automation tools for running MANET protocol comparison experiments.

## Quick Start

```bash
# Install dependencies
pip3 install pandas matplotlib

# Run basic comparison
cd /home/ahmed/Underwater-MANET-Routing-protocols-GAODV-
python3 scripts/run_all.py --protocols GAODV AODV --runs 10

# Generate plots
python3 scripts/plot_results.py
```

## Tools

### `run_all.py` - Experiment Automation

Automates running NS-3 simulations with parameter sweeping.

**Features:**
- Auto-detects NS-3 installation
- Protocol parameter sweeping (protocols, nodes, txPower)
- Progress tracking with PDR extraction
- Result aggregation to CSV

**Examples:**

```bash
# Compare GAODV vs AODV
python3 scripts/run_all.py --protocols GAODV AODV --runs 10

# Parameter sweep: nodes and transmission power
python3 scripts/run_all.py --protocols GAODV --nodes 25 50 100 --txpower 5.0 7.5 10.0 --runs 5

# Quick test (skip build, 3 runs)
python3 scripts/run_all.py --protocols GAODV --runs 3 --skip-build

# Use specific NS-3 installation
python3 scripts/run_all.py --protocols GAODV --ns3-path /path/to/ns-3.40
```

**Parameters:**
- `--protocols`: Routing protocols to test (GAODV, AODV, OLSR, DSDV, DSR)
- `--nodes`: Number of network nodes (default: 50)
- `--txpower`: Transmission power in dBm (default: 7.5)
- `--runs`: Number of runs per configuration (default: 5)
- `--ns3-path`: Path to NS-3 installation (auto-detected if omitted)
- `--skip-build`: Skip NS-3 build step
- `--no-aggregate`: Skip result aggregation

**Output:**
- Individual CSV files: `results_<PROTOCOL>.csv`
- Aggregated results: `results/aggregated_results.csv`

---

### `plot_results.py` - Result Visualization

Generates comparison plots from aggregated results.

**Features:**
- PDR comparison (bar chart with error bars)
- Energy consumption comparison
- Energy efficiency metric (PDR/Energy)
- PDR vs Energy trade-off scatter plot
- Statistics summary table

**Examples:**

```bash
# Generate all plots (uses default paths)
python3 scripts/plot_results.py

# Specify custom input/output paths
python3 scripts/plot_results.py --input results/aggregated_results.csv --output graphs/
```

**Output:**
- `graphs/pdr_comparison.png` - PDR comparison across protocols
- `graphs/energy_comparison.png` - Energy consumption comparison
- `graphs/efficiency_comparison.png` - Energy efficiency comparison
- `graphs/tradeoff_scatter.png` - PDR vs Energy scatter plot
- `graphs/statistics.csv` - Statistical summary table

---

## Workflow

### Full Experiment Workflow

```bash
# 1. Run experiments with parameter sweeps
python3 scripts/run_all.py --protocols GAODV AODV OLSR --nodes 25 50 --txpower 5.0 7.5 --runs 10

# 2. Generate plots and statistics
python3 scripts/plot_results.py

# 3. View results
cat results/aggregated_results.csv
cat graphs/statistics.csv
open graphs/*.png  # or use image viewer
```

### Quick Test Workflow

```bash
# Fast test with minimal runs
python3 scripts/run_all.py --protocols GAODV AODV --runs 3 --skip-build
python3 scripts/plot_results.py
```

---

## File Structure

```
Underwater-MANET-Routing-protocols-GAODV-/
├── scripts/
│   ├── run_all.py          # Experiment automation
│   ├── plot_results.py     # Result visualization
│   └── README.md           # This file
├── src/
│   └── manet-compare*      # NS-3 simulation source
├── results/
│   ├── results_GAODV.csv   # Per-protocol results
│   ├── results_AODV.csv
│   └── aggregated_results.csv  # Combined results
└── graphs/
    ├── pdr_comparison.png
    ├── energy_comparison.png
    ├── efficiency_comparison.png
    ├── tradeoff_scatter.png
    └── statistics.csv
```

---

## Dependencies

```bash
# Install required Python packages
pip3 install pandas matplotlib

# Or using requirements.txt (if created)
pip3 install -r requirements.txt
```

---

## Troubleshooting

### NS-3 Not Found

If auto-detection fails:
```bash
python3 scripts/run_all.py --ns3-path /path/to/your/ns-3.XX
```

### Build Errors

If build fails, ensure NS-3 is properly configured:
```bash
cd /path/to/ns-3.XX
./ns3 configure
./ns3 build
```

### No Results to Plot

Ensure experiments ran successfully:
```bash
ls results_*.csv  # Should show protocol CSV files
cat results_GAODV.csv  # Verify data exists
```

### Empty Plots

Check that aggregated results exist:
```bash
cat results/aggregated_results.csv
```

---

## Methodology

### PDR Calculation

Based on NS3-Research methodology:
- Tracks **unique packet UIDs** using `std::set<uint64_t>`
- Sender callback counts transmitted packets
- Sink callback deduplicates received packets
- PDR = unique_received / total_sent

This correctly handles multipath duplicates in protocols like GAODV.

### Energy Tracking

- Stores initial energy before `Simulator::Run()`
- Queries remaining energy after `Simulator::Destroy()`
- Energy consumed = initial - remaining

### Result Aggregation

- Each protocol run writes to `results_<PROTOCOL>.csv`
- `run_all.py` combines all results into `aggregated_results.csv`
- Plotting tool analyzes aggregated data with statistics

---

## Advanced Usage

### Custom Parameter Combinations

```bash
# Test specific scenarios
python3 scripts/run_all.py \
    --protocols GAODV AODV \
    --nodes 25 50 75 100 \
    --txpower 5.0 6.0 7.0 8.0 9.0 10.0 \
    --runs 20
```

### Continuous Integration

```bash
#!/bin/bash
# run_experiments.sh

# Run overnight experiments
python3 scripts/run_all.py --protocols GAODV AODV OLSR DSDV --runs 50

# Generate plots
python3 scripts/plot_results.py

# Archive results
tar -czf results_$(date +%Y%m%d).tar.gz results/ graphs/
```

---

## Contact & Support

For issues or questions about the automation tools, please check:
1. This README
2. Main project README.md
3. Implementation plan document

---

**Last Updated:** 2026-01-08
