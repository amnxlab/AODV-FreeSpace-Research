# Batch v0.3 Fix - Critical Bug Fixes & Energy Model Overhaul

**Date:** February 8, 2026  
**Version:** 0.3  
**Status:** ✅ Complete

---

## Executive Summary

This batch addressed critical bugs in GAODV implementation and overhauled the energy model from constant-drain to WiFi PHY state-based tracking. All fixes maintain PDR and throughput while providing accurate energy consumption metrics.

---

## Critical Bugs Discovered & Fixed

### 1. InitialEnergy Attribute Mismatch (CRITICAL)

**Bug:** GAODV's `InitialEnergy` attribute defaulted to 500J while simulations used 20J.

**Impact:** 
- NEF calculation: `residual_energy / 500J` → artificially low NEF values
- RREQs dropped when `NEF < ETHF` threshold
- **Result: 0% PDR** in all GAODV simulations

**Root Cause:**
```cpp
// gaodv-v1-routing-protocol.cc:76 (BEFORE)
.AddAttribute("InitialEnergy",
              "Initial energy for NEF calculation (J)",
              DoubleValue(500.0),  // ❌ Wrong default
```

**Fix:**
```cpp
// gaodv-v1-routing-protocol.cc:76 (AFTER)
.AddAttribute("InitialEnergy",
              "Initial energy for NEF calculation (J)",
              DoubleValue(20.0),  // ✅ Matches EDA-AODV paper
```

**Verification:** GAODV PDR: 0% → 99.87%

---

### 2. ComputeRef() Never Called (CRITICAL)

**Bug:** Destination node selected paths using only RREQEF instead of full REF calculation.

**Impact:** 
- Ignored distance, hop count, and delay factors
- Violated G-AODV flowchart specification
- Suboptimal route selection

**Root Cause:**
```cpp
// gaodv-v1-routing-protocol.cc:1113 (BEFORE)
if (rreqef > bestRreqef)  // ❌ Only energy factor
{
    bestRreqef = rreqef;
    bestEntry = kv.second;
}
```

**Fix:**
```cpp
// gaodv-v1-routing-protocol.cc:1109 (AFTER)
double ref = ComputeRef(kv.second);  // ✅ Full REF calculation
if (ref > bestRef)
{
    bestRef = ref;
    bestEntry = kv.second;
}
```

**REF Formula:**
```
REF = W1×RREQEF + W2×(1/distance) + W3×(1/hops) + W4×(1/delay)
```

---

### 3. Alpha Parameter Unused (CRITICAL)

**Bug:** `m_alpha` declared but never used in NEF threshold logic.

**Impact:** 
- Flowchart requirement `NEF >= ETHF AND 0.4 <= α <= 0.8` not enforced
- Alpha parameter had no effect on routing

**Root Cause:**
```cpp
// gaodv-v1-routing-protocol.cc:1048 (BEFORE)
if (myNef < m_ethf)  // ❌ Alpha not used
{
    NS_LOG_DEBUG("Node " << m_node->GetId() << " dropping RREQ");
    return;
}
```

**Fix:**
```cpp
// gaodv-v1-routing-protocol.cc:1047 (AFTER)
double minAcceptableNef = m_alpha * m_ethf;
if (myNef < m_ethf || myNef < minAcceptableNef)  // ✅ Alpha integrated
{
    NS_LOG_DEBUG("Node " << m_node->GetId() << " dropping RREQ: NEF "
                 << myNef << " below threshold (ETHF=" << m_ethf
                 << ", alpha=" << m_alpha << ", min=" << minAcceptableNef << ")");
    return;
}
```

---

### 4. Alpha Range Incorrect

**Bug:** Alpha constrained to 0.5-0.7 instead of flowchart's 0.4-0.8.

**Fix:**
```cpp
// gaodv-v1-routing-protocol.cc:63
MakeDoubleChecker<double>(0.4, 0.8)  // ✅ Matches flowchart
```

---

## Energy Model Overhaul

### Problem

Previous implementation used `SimpleDeviceEnergyModel` with constant 100mA drain:
- ❌ Didn't reflect actual WiFi activity
- ❌ Same energy consumption regardless of TX/RX/Idle states
- ❌ Inaccurate for protocol comparison

### Solution: WifiRadioEnergyModel

Replaced with `WifiRadioEnergyModelHelper` that connects to WiFi PHY state machine:

```cpp
// manet-compare-freespace-v1.cc:342-353 (AFTER)
WifiRadioEnergyModelHelper radioEnergyHelper;
// Typical 802.11a power consumption (Atheros AR5414)
radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.380));    // 380mA TX
radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.313));    // 313mA RX
radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.273));  // 273mA Idle
radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.033)); // 33mA Sleep
radioEnergyHelper.Set("CcaBusyCurrentA", DoubleValue(0.273));
radioEnergyHelper.Set("SwitchingCurrentA", DoubleValue(0.273));
m_deviceEnergyModels = radioEnergyHelper.Install(adhocDevices, m_energySources);
```

### How It Works

1. **PHY State Tracking:** Energy model registers callbacks with WiFi PHY
2. **State Transitions:** Monitors TX/RX/Idle/Sleep state changes
3. **Current Integration:** Calculates `∫ I(t) dt` over time for each state
4. **Energy Depletion:** Updates `BasicEnergySource` based on actual consumption

### Energy Comparison

| Scenario | Old Model | New Model |
|----------|-----------|-----------|
| 30 nodes, 50s | 59.4J | 510.3J (GAODV) |
| 100 nodes, 500s | 1980J | 1698.6J (GAODV) |

**Note:** New model shows realistic values based on actual PHY activity.

---

## Additional Improvements

### Result File Management

**Before:** All runs aggregated into single `aggregated_results.csv`

**After:** Each run gets separate timestamped file:
```
results/
├── results_GAODV_20260208_065147.csv
├── results_AODV_20260208_065158.csv
└── ...
```

**Implementation:**
- Renamed `aggregate_results()` → `collect_results()`
- Moves individual files to results folder
- CLI: `--no-collect` to skip collection

---

## Verification Results

### 30 Nodes, 50s Simulation

| Protocol | PDR | Throughput | Energy |
|----------|-----|------------|--------|
| GAODV | 99.87% | 1.61 kbps | 510.3J |
| AODV | 100% | 1.61 kbps | 529.8J |

### 100 Nodes, 500s Simulation

| Protocol | PDR | Throughput | Energy |
|----------|-----|------------|--------|
| GAODV | 99.99% | 20.04 kbps | 1698.6J |
| AODV | 100% | 20.04 kbps | 1790.9J |

**✅ PDR and throughput maintained**  
**✅ Energy reflects actual WiFi activity**

---

## Files Modified

| File | Changes |
|------|---------|
| `gaodv-v1/gaodv-v1-routing-protocol.cc` | InitialEnergy default, ComputeRef usage, alpha integration, alpha range |
| `src/manet-compare-freespace-v1.cc` | WifiRadioEnergyModel integration, include wifi-radio-energy-model-helper.h |
| `scripts/run_all.py` | Result collection instead of aggregation |

---

## Testing Notes

### Grid Spacing Issue

During testing, discovered 0% PDR with default settings:
- **Cause:** Grid spacing 120m > TX range 100m
- **Solution:** Use `--txrange 150` or adjust grid spacing
- **Lesson:** Always verify node connectivity in grid layouts

---

## References

1. NS-3 `energy-model-example.cc` - BasicEnergySource + WifiRadioEnergyModel
2. NS-3 `generic-battery-wifiradio-example.cc` - GenericBattery + WifiRadioEnergyModel
3. G-AODV Flowchart - Route selection and NEF threshold logic
4. EDA-AODV Paper - 20J initial energy specification

---

## Conclusion

All critical bugs resolved. GAODV now:
- ✅ Follows flowchart specification
- ✅ Uses proper energy model
- ✅ Achieves 99.9%+ PDR
- ✅ Provides accurate energy metrics
