# Enhanced Visualization System

## Overview
Publication-quality visualization system adapted from NS3-Research UWSN project for GAODV protocol comparison.

**Status**: ✅ Implemented (Phase 1)  
**Version**: 1.0  
**Date**: February 9, 2026

---

## Features

### 1. **Energy Breakdown Stacked Bar Chart** (Phase 0)
- **Location**: `dashboard/api/visualizer_enhanced.py` → `generate_energy_breakdown_stacked()`
- **Purpose**: Shows routing/data/idle energy components for each protocol
- **Highlights**:
  - Color-coded components: Routing (red), Data (blue), Idle (gray)
  - Percentage labels inside each segment
  - Total energy label on top of each bar
  - Demonstrates GAODV's energy efficiency advantage

### 2. **Routing Overhead Comparison**
- **Location**: `dashboard/api/visualizer_enhanced.py` → `generate_routing_overhead_comparison()`
- **Purpose**: Compares routing vs data packet counts
- **Highlights**:
  - Grouped bars with error bars
  - Mean ± std labels
  - Shows protocol efficiency in packet overhead

### 3. **Protocol Comparison Matrix** (Adaptive)
- **Location**: `dashboard/api/visualizer_enhanced.py` → `generate_all_charts()`
- **Purpose**: Comprehensive analysis across parameter variations
- **Generated When**: CSV has varying X-variables (nodes, txPower, simtime)
- **Chart Matrix**:
  ```
  X-vars: nodes, txPower, simtime
  Y-vars: pdr_percent, consumedEnergy, energy_efficiency, routing_overhead_pct
  ```
- **Auto-generates**: Up to 12 charts (3 X-vars × 4 Y-vars)

### 4. **Statistics Summary**
- **Location**: Dashboard UI (top of charts section)
- **Metrics per Protocol**:
  - PDR: Mean ± Std (%)
  - Energy: Mean ± Std (J)
  - Efficiency: PDR/J ratio
  - Routing Energy: Percentage breakdown

---

## Technical Specifications

### Chart Quality
- **DPI**: 300 (publication quality)
- **Format**: PNG, base64 encoded for web
- **Size**: 12×7 inches (3600×2100 px)
- **Backend**: matplotlib (Agg)

### Color Scheme (Color-blind Safe)
**Protocols:**
- GAODV: `#1565C0` (Strong Blue) - energy-aware
- AODV: `#C62828` (Dark Red) - baseline
- DSDV: `#2E7D32` (Dark Green) - proactive
- DSR: `#F9A825` (Golden Yellow) - source routing

**Energy Components:**
- Routing: `#C62828` (Dark Red) - overhead
- Data: `#1565C0` (Strong Blue) - useful work
- Idle: `#757575` (Gray) - waste

### Styling (from plot_comprehensive.py)
- **Fonts**: Bold labels and titles
  - Title: 14pt
  - Axis labels: 12pt
  - Tick labels: 11pt
  - Legend: 11pt
- **Grid**: Dashed lines, 40% alpha, behind bars
- **Error bars**: 3px cap size
- **Value labels**: White background bbox, mean±std format
- **Y-axis limits**: Bottom=0, Top=max*1.25 (space for labels)

---

## Implementation

### Files Created
1. **`dashboard/api/visualizer_enhanced.py`** (630 lines)
   - `calculate_metrics()` - Compute PDR%, efficiency, overhead
   - `generate_grouped_bar_plot()` - Vertical grouped bars
   - `generate_energy_breakdown_stacked()` - Phase 0 breakdown
   - `generate_routing_overhead_comparison()` - Packet analysis
   - `generate_all_charts()` - Main orchestrator
   - `calculate_statistics()` - Comprehensive stats

### Files Modified
1. **`dashboard/server.py`**
   - Line 23: Import `visualizer_enhanced`
   - Line 241: Use `generate_all_charts()` in `/api/results/visualize`

2. **`dashboard/public/index.html`**
   - Lines 2004-2065: Enhanced `displayCharts()` function
   - Statistics panel at top
   - Dynamic matrix chart detection
   - Chart titles and ordering

---

## API Usage

### Endpoint: `/api/results/visualize/<filename>`

**Request:**
```http
GET /api/results/visualize/results_session_20260209_231541.csv
```

**Response:**
```json
{
  "statistics": {
    "GAODV": {
      "runs": 5,
      "pdr_mean": 99.90,
      "pdr_std": 0.05,
      "energy_mean": 279.31,
      "energy_std": 1.24,
      "efficiency_mean": 0.3577,
      "routing_energy_pct": 25.4,
      "data_energy_pct": 68.2,
      "idle_energy_pct": 6.4
    },
    "AODV": { ... }
  },
  "energy_breakdown": "data:image/png;base64,iVBORw0KGgoAAAANSU...",
  "routing_overhead": "data:image/png;base64,iVBORw0KGgoAAAANSU...",
  "pdr_percent_by_nodes": "data:image/png;base64,..." 
}
```

### Query Parameters
```http
GET /api/results/visualize/results.csv?charts=energy_breakdown,routing_overhead
```

**Chart Types:**
- `all` - All charts (default)
- `energy_breakdown` - Energy component breakdown
- `routing_overhead` - Packet comparison
- `matrix` - Parameter variation matrix
- `pdr` - Legacy PDR chart
- `energy` - Legacy energy chart
- `efficiency` - Legacy efficiency chart

---

## Comparison: Old vs Enhanced

| Feature | Old Visualizer | Enhanced Visualizer |
|---------|---------------|---------------------|
| **Energy Breakdown** | ❌ No | ✅ Stacked bar with % labels |
| **Routing Overhead** | ❌ No | ✅ Grouped bar comparison |
| **Matrix Charts** | ❌ No | ✅ Auto-generated for varying params |
| **Statistics** | Basic | Comprehensive (mean, std, min, max) |
| **Chart Quality** | 100 DPI | 300 DPI (publication) |
| **Value Labels** | ❌ No | ✅ Mean ± std on bars |
| **Error Bars** | ❌ No | ✅ With cap size |
| **Color Scheme** | Basic | Color-blind safe palette |
| **Phase 0 Support** | ❌ No | ✅ Routing/data/idle breakdown |

---

## Validation Results

### Test CSV: `results_session_20260209_231541.csv`
- **Rows**: 10 (5 GAODV + 5 AODV runs)
- **Nodes**: 200 (fixed)
- **Generated Charts**: 2 + statistics
  - Energy breakdown ✓
  - Routing overhead ✓
  - Matrix charts: 0 (no varying X-vars)

### Energy Savings Validation
```
GAODV: 279.31J ± 0.00
AODV:  657.09J ± 0.00
Savings: 57.5%
```

### Performance
- **Chart Generation**: ~2-3 seconds for all charts
- **Base64 Encoding**: ~200KB per chart (300 DPI)
- **Server Response**: <5 seconds total

---

## Future Enhancements (Phase 2)

### Planned Features
1. **Protocol Radar Chart**
   - Multi-dimensional comparison
   - 5-axis: PDR, Energy, Efficiency, Overhead, Throughput
   - Shaded area per protocol

2. **Scalability Curves**
   - Line charts: Energy/PDR vs nodes
   - Trend analysis with regression
   - Confidence intervals

3. **Statistical Comparison Table**
   - T-tests between protocols
   - P-values and confidence intervals
   - Effect size (Cohen's d)

4. **Interactive Plotly Charts**
   - Hover tooltips with run details
   - Zoom and pan
   - Export to SVG/PDF

5. **PDF Report Generation**
   - Multi-page comprehensive report
   - All charts + statistics
   - Executive summary

---

## References

### Adapted From
- **Source**: `/home/ahmed/NS3-Research/scripts/plot_comprehensive.py`
- **Project**: UWSN Protocol Analysis (11 protocols)
- **Key Features Adopted**:
  - Vertical grouped bars with error bars
  - Matrix-based chart generation
  - 300 DPI publication quality
  - Color-blind safe palette
  - Mean ± std value labels

### Key Differences
| Aspect | NS3-Research | GAODV Dashboard |
|--------|--------------|-----------------|
| **Protocols** | 11 UWSN | 4 MANET (GAODV/AODV/DSDV/DSR) |
| **Energy Model** | Generic | Phase 0 (routing/data/idle) |
| **Metrics** | bits/J, delay, lifetime | PDR/J, overhead%, residual |
| **X-variables** | nodes, depth, width, mobility, range | nodes, txPower, simtime |
| **Output** | Static PNG files | Base64 web delivery |
| **Integration** | Standalone script | Flask API endpoint |

---

## Troubleshooting

### No Matrix Charts Generated
**Cause**: CSV has only one unique value per X-variable  
**Solution**: Run simulations with varying nodes/txPower/simtime

### Low Chart Quality
**Cause**: DPI setting changed  
**Solution**: Check `CHART_CONFIG['dpi'] = 300` in visualizer_enhanced.py

### Missing Energy Breakdown
**Cause**: CSV missing routingEnergyJ/dataEnergyJ/idleEnergyJ columns  
**Solution**: Verify Phase 0 energy calculation in simulation

### Statistics Not Showing
**Cause**: Missing 'protocol' column in CSV  
**Solution**: Use consolidated CSV format with protocol column

---

## Code Examples

### Generate All Charts (Python)
```python
from dashboard.api.visualizer_enhanced import generate_all_charts

csv_path = '/path/to/results_session_20260209.csv'
result = generate_all_charts(csv_path, chart_types=['all'])

# Access charts
energy_breakdown = result['energy_breakdown']  # base64 PNG
routing_overhead = result['routing_overhead']  # base64 PNG
statistics = result['statistics']              # dict

# Check for errors
if 'error' in result:
    print(f"Error: {result['error']}")
```

### Custom Chart Type Selection
```python
# Generate only specific charts
result = generate_all_charts(csv_path, 
                            chart_types=['energy_breakdown', 'routing_overhead'])
```

### Display in Web Browser (JavaScript)
```javascript
fetch('/api/results/visualize/results_session_20260209.csv')
    .then(response => response.json())
    .then(data => {
        // Display energy breakdown
        if (data.energy_breakdown) {
            document.getElementById('energyChart').src = data.energy_breakdown;
        }
        
        // Display statistics
        Object.entries(data.statistics).forEach(([protocol, stats]) => {
            console.log(`${protocol}: ${stats.energy_mean}J, PDR: ${stats.pdr_mean}%`);
        });
    });
```

---

## Metrics Reference

### Calculated Metrics

1. **pdr_percent**
   ```python
   pdr_percent = pdr * 100  # if pdr ∈ [0, 1]
   ```

2. **energy_efficiency**
   ```python
   energy_efficiency = pdr_percent / consumedEnergy  # PDR per Joule
   ```

3. **routing_overhead_pct**
   ```python
   total_packets = routingPackets + dataPackets
   routing_overhead_pct = (routingPackets / total_packets) * 100
   ```

4. **avg_residual_energy_per_node**
   ```python
   avg_residual_energy_per_node = residualEnergy / nodes
   ```

---

## Changelog

### v1.0 (February 9, 2026)
- ✅ Created enhanced visualizer module (630 lines)
- ✅ Implemented energy breakdown stacked chart
- ✅ Implemented routing overhead comparison
- ✅ Implemented matrix chart generation
- ✅ Updated dashboard UI for new charts
- ✅ Integrated with Flask API
- ✅ Tested with real simulation data
- ✅ Validated GAODV 57.5% energy savings

### Future Versions
- v1.1: Protocol radar chart
- v1.2: Scalability curves
- v1.3: Statistical comparison tables
- v2.0: Interactive Plotly charts + PDF reports

---

## Maintainer Notes

**Module**: `dashboard/api/visualizer_enhanced.py`  
**Maintainer**: GAODV Research Team  
**Last Updated**: February 9, 2026  
**Dependencies**: pandas, matplotlib, numpy  
**Python Version**: 3.8+

**Critical Functions:**
- `generate_all_charts()` - Main entry point
- `calculate_metrics()` - Add derived metrics
- `fig_to_base64()` - Web delivery

**Configuration:**
- Chart settings: `CHART_CONFIG` dict
- Protocol colors: `PROTOCOL_COLORS` dict
- Energy colors: `ENERGY_COMPONENT_COLORS` dict
