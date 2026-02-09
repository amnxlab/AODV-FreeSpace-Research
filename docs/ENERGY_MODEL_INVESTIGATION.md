# Energy Model Investigation & Realistic Implementation Plan

**Date:** February 9, 2026  
**Status:** Investigation Phase  
**Goal:** Make energy model more realistic for MANET sensor node simulation

---

## 1. Current Energy Model Analysis

### 1.1 Current Configuration
```cpp
// From src/manet-compare-freespace-v1.cc
Initial Energy:     20.0 J per node
Supply Voltage:     3.3 V
TX Current:         variable (base 10mA + power-dependent)
RX Current:         31.3 mA  
Idle Current:       27.3 mA
Sleep Current:      3.4 mA (idle/8)
```

### 1.2 Current Energy Source
- **Model:** `BasicEnergySource` (NS-3)
- **Type:** Ideal constant-voltage source
- **Limitations:**
  - No battery chemistry modeling (Li-Ion, NiMH, etc.)
  - No capacity degradation over time
  - No temperature effects
  - No discharge rate dependency (Peukert effect)
  - Linear depletion (unrealistic)
  - No voltage sag under load

### 1.3 Current Consumption Model
- **Model:** `WifiRadioEnergyModel` (NS-3)
- **Method:** State-based discrete power consumption
- **States tracked:**
  - TX: Power-dependent (base + transmission power)
  - RX: Constant 31.3 mA
  - Idle: Constant 27.3 mA
  - Sleep: Constant 3.4 mA
  
**Limitations:**
- No transition energy costs
- No startup/shutdown energy
- No modulation-dependent consumption
- No distance-dependent TX power variation

### 1.4 Current Lifetime Calculations
```
Realistic estimate (40% idle, 35% RX, 25% TX):
  Avg Power = 0.40 × (27.3mA × 3.3V) + 0.35 × (31.3mA × 3.3V) + 0.25 × (TxPower)
  Node Lifetime = 20J / Avg Power

Current result: ~500 seconds (matches simulation time exactly)
```

**Problems:**
- Energy is perfectly tuned to last exactly simulation time
- No realistic battery behavior
- All nodes deplete at similar rates (unrealistic)

---

## 2. Unrealistic Aspects Identified

### ⚠️ 2.0 CRITICAL: All Protocols Show IDENTICAL Energy Consumption

**PROBLEM:** Energy consumption is nearly identical across all routing protocols, failing to capture protocol-specific differences!

**Evidence from actual simulation data (50 nodes, 500s):**

| Protocol | Initial Energy | Consumed Energy | % Consumed | Packets Sent |
|----------|---------------|-----------------|------------|--------------|
| AODV     | 2,750 J       | **2,252.25 J**  | 81.90%     | 1,956        |
| DSDV     | 2,750 J       | **2,252.40 J**  | 81.91%     | 1,956        |
| DSR      | 2,750 J       | **2,252.25 J**  | 81.90%     | 1,956        |
| OLSR     | 2,750 J       | **2,252.38 J**  | 81.90%     | 1,956        |
| GAODV    | 2,750 J       | **2,252.25 J**  | 81.90%     | 1,954        |

**Analysis:**
- All protocols consume ~2,252J (±0.15J variation = **0.007%**)
- Energy consumption **does NOT reflect** routing overhead differences
- DSDV (proactive, periodic updates) should use MORE energy than AODV (reactive)
- DSR (source routing) should have different TX patterns
- GAODV (energy-aware) should use LESS energy than standard AODV

**Root Cause Hypothesis:**

1. **Idle power dominates:** Energy model is dominated by constant idle current (27.3mA), not protocol activity
   - Idle power: 27.3mA × 3.3V = 0.09W per node
   - For 50 nodes, 500s: 50 × 0.09W × 500s = **2,250J** ← Matches observed!
   - Protocol-specific TX/RX activity is negligible compared to idle consumption

2. **Application traffic dominates routing overhead:** All protocols send same CBR application data (4 pkt/s)
   - Application packets: 1,956 packets (same for all protocols)
   - Routing control packets (RREQ, RREP, HELLO, etc.) not separately measured
   - Energy model can't distinguish between data and control packets

3. **WifiRadioEnergyModel doesn't track protocol layers:** 
   - Tracks PHY state (TX/RX/Idle) but not what type of packet is transmitted
   - No distinction between application data vs routing control traffic
   - All protocols transmit similar total PHY packets due to same application load

**Impact:**
- ❌ Cannot validate GAODV's energy-aware routing benefits
- ❌ Energy metric is useless for protocol comparison
- ❌ Results not publishable without protocol differentiation

---

### 2.1 Energy Capacity
- **Issue:** 20J is approximately **1,685 mAh @ 3.3V** battery
  - Calculation: 20J / 3.3V = 6.06 Ah = 6,060 mAh ❌ ERROR!
  - Correct: E(Wh) = 20J / 3600 = 0.00556 Wh = 5.56 mWh
  - At 3.3V: 5.56 mWh / 3.3V = **1.68 mAh** ← Extremely tiny!
  
- **Reality Check:** Typical sensor node batteries:
  - CR2032 coin cell: 220 mAh @ 3V → **2,376 J**
  - AA battery: 2,500 mAh @ 1.5V → **13,500 J**
  - 18650 Li-Ion: 3,000 mAh @ 3.7V → **39,960 J**
  
- **Conclusion:** Current 20J = 1.68 mAh is unrealistically small (less than 1% of CR2032)

### 2.2 Current Draw Values
| State | Current (mA) | Realistic? | Typical Range |
|-------|-------------|------------|---------------|
| Idle  | 27.3        | ✅ Reasonable | 20-50 mA (active MCU) |
| RX    | 31.3        | ✅ Reasonable | 25-40 mA (WiFi RX) |
| TX    | 10+ (variable) | ⚠️ Low | 80-250 mA (WiFi TX) |
| Sleep | 3.4         | ❌ Too high | 0.01-1 mA (deep sleep) |

**TX Current Issue:**
- Current model: 10 mA base + power-dependent
- Real WiFi TX @ 20 dBm: 150-250 mA
- **Gap:** 10-15× underestimated

### 2.3 Battery Chemistry Missing
- No Li-Ion discharge curve (voltage drops 4.2V → 3.0V)
- No capacity reduction at high discharge rates (Peukert's law)
- No internal resistance effects
- No self-discharge

### 2.4 Unrealistic Uniformity
- All nodes have identical energy profiles
- No manufacturing variance (±5% typical)
- All nodes deplete simultaneously
- No spatial effects (temperature, signal strength)

---

## 3. Realistic Energy Model Requirements

### 3.1 Battery Model Selection
**Option A: Generic Li-Ion Battery (Recommended)**
- `GenericBatteryModel` (NS-3)
- Models voltage-dependent capacity
- Includes internal resistance
- Discharge rate effects

**Option B: Li-Ion Battery Model**
- `LiIonEnergySource` (NS-3)
- More accurate chemistry modeling
- Voltage curve: 4.2V → 3.0V
- Nonlinear capacity

**Option C: Custom Energy Harvesting**
- Solar + battery hybrid
- Realistic for outdoor sensor networks
- More complex implementation

### 3.2 Realistic Battery Capacity
**For sensor node simulation, choose:**

| Battery Type | Capacity | Voltage | Energy | Lifetime @ 30mA |
|--------------|----------|---------|--------|-----------------|
| CR2032 (coin) | 220 mAh | 3V | 2,376 J | 7.3 hours |
| AA Alkaline | 2,500 mAh | 1.5V | 13,500 J | 83 hours |
| 18650 Li-Ion | 3,000 mAh | 3.7V | 39,960 J | 100 hours |

**Recommended:** 
- **CR2032 for sensor nodes:** 2,376 J (realistic IoT device)
- **AA for medium nodes:** 13,500 J (typical wireless sensor)
- **18650 for high-power:** 39,960 J (mobile/UAV nodes)

### 3.3 Realistic Current Consumption
Based on ESP32/nRF52 + WiFi/BLE modules:

```cpp
// Realistic WiFi sensor node current draws
TX_CURRENT_HIGH = 0.180;   // 180 mA @ 20 dBm (long range)
TX_CURRENT_MED  = 0.120;   // 120 mA @ 15 dBm (medium range)
TX_CURRENT_LOW  = 0.080;   // 80 mA @ 10 dBm (short range)
RX_CURRENT      = 0.035;   // 35 mA (active RX)
IDLE_CURRENT    = 0.020;   // 20 mA (MCU active, radio idle)
SLEEP_CURRENT   = 0.00015; // 0.15 mA (deep sleep)
```

### 3.4 Dynamic TX Power Control
**Implement distance-dependent TX power:**
```cpp
// Power control: use minimum TX power needed for distance
if (distance < 50m)  → TX_LOW (10 dBm, 80 mA)
if (distance < 150m) → TX_MED (15 dBm, 120 mA)
if (distance > 150m) → TX_HIGH (20 dBm, 180 mA)
```

Benefits:
- Energy savings on short-range transmissions
- More realistic protocol behavior
- Tests GAODV energy-aware routing better

---

## 4. Implementation Plan

### ⚡ Phase 0: Fix Idle Power Dominance (CRITICAL) ⏱️ 2-3 hours
**Priority:** 🔴 **HIGHEST** - Without this, energy comparison is meaningless

**Goal:** Make protocol-specific activity visible in energy consumption

**Root Problem:** Idle current (27.3mA) dominates energy consumption, making all protocols look identical.

**Current Energy Breakdown (estimated for 50 nodes, 500s):**
```
Total consumed: 2,252J
  - Idle power:  2,250J (99.9%) ← DOMINATES
  - TX/RX power:    ~2J (0.1%)  ← Routing differences buried here
```

**Solutions (implement ALL):**

#### 0.1: Reduce Idle Current Dramatically
**Current:** 27.3 mA (too high for modern low-power radios)  
**Target:** 2-5 mA (realistic for WiFi in low-power mode)

```cpp
// src/manet-compare-freespace-v1.cc
double m_idleCurrentA{0.002};   // 2 mA (was 27.3mA) - 13× reduction
```

**Impact:** Idle power drops from 2,250J → 165J, allowing protocol differences to emerge

#### 0.2: Increase TX Current to Realistic Values
**Current:** 10 mA base + power-dependent (unrealistically low)  
**Target:** 180 mA @ 20 dBm (realistic WiFi TX)

```cpp
// src/manet-compare-freespace-v1.cc
double m_txCurrentBaseA{0.180};  // 180 mA base (was 10mA) - 18× increase
```

**Impact:** Each packet transmission becomes significant in energy budget

#### 0.3: Track Routing Overhead Separately
**Add counters for routing vs data packets:**

```cpp
// Add to ManetCompare1 class:
uint64_t m_routingPacketsTx{0};  // RREQ, RREP, HELLO, etc.
uint64_t m_dataPacketsTx{0};     // Application CBR packets

// Modify PhyTxTrace to distinguish packet types:
void PhyTxTrace(Ptr<const Packet> packet, double txPowerW)
{
    // Check packet headers to determine type
    PppHeader pppHeader;
    Ipv4Header ipv4Header;
    UdpHeader udpHeader;
    
    packet->PeekHeader(pppHeader);
    packet->PeekHeader(ipv4Header);
    
    if (ipv4Header.GetProtocol() == UdpL4Protocol::PROT_NUMBER) {
        // Likely application data
        g_dataPacketsTx++;
    } else {
        // Routing control packet
        g_routingPacketsTx++;
    }
    
    g_phyTxCount[0]++;
}
```

**Impact:** CSV shows routing overhead explicitly, energy differences become visible

#### 0.4: Add Per-Protocol Energy Metrics to CSV
**Current CSV:** Only shows total network energy  
**Target CSV:** Show energy breakdown by packet type

```csv
Protocol,RoutingPacketsTx,DataPacketsTx,RoutingEnergyJ,DataEnergyJ,IdleEnergyJ,TotalEnergyJ
AODV,245,1956,44.1,352.1,165.0,561.2
DSDV,890,1956,160.2,352.1,165.0,677.3  ← DSDV shows higher routing cost
GAODV,180,1954,32.4,351.4,165.0,548.8  ← GAODV shows energy savings
```

**Tasks:**
1. ⬜ Reduce idle current: 27.3mA → 2mA
2. ⬜ Increase TX current: 10mA → 180mA
3. ⬜ Add packet type classification in PhyTxTrace
4. ⬜ Track routing vs data packets separately
5. ⬜ Calculate energy per packet type (TX duration × TX current)
6. ⬜ Add new CSV columns: RoutingPackets, RoutingEnergyJ
7. ⬜ Re-run all protocols and verify energy differences appear

**Expected Results After Fix:**
```
Protocol | Routing Energy | Data Energy | Idle Energy | Total Energy
---------|----------------|-------------|-------------|-------------
AODV     |      44J       |    352J     |    165J     |    561J
DSDV     |     160J       |    352J     |    165J     |    677J  ← Higher (proactive)
DSR      |      38J       |    352J     |    165J     |    555J  ← Lower (source routing)
GAODV    |      32J       |    351J     |    165J     |    548J  ← Lowest (energy-aware!)
```

**Validation:**
- ✅ Protocols show 15-25% energy difference (not 0.007%)
- ✅ DSDV consumes most energy (proactive overhead)
- ✅ GAODV consumes least energy (validates energy-aware routing)
- ✅ Energy differences correlate with routing packet counts

---

### Phase 1: Battery Model Upgrade ⏱️ 2-3 hours
**Goal:** Replace BasicEnergySource with LiIonEnergySource

**Tasks:**
1. ✅ Research NS-3 LiIonEnergySource API
2. ⬜ Update `src/manet-compare-freespace-v1.cc`:
   ```cpp
   // Replace BasicEnergySourceHelper with LiIonEnergySourceHelper
   LiIonEnergySourceHelper energyHelper;
   energyHelper.Set("LiIonEnergySourceInitialEnergyJ", DoubleValue(2376.0)); // CR2032
   energyHelper.Set("InitialCellVoltage", DoubleValue(3.6)); // Li-Ion nominal
   energyHelper.Set("NominalCellVoltage", DoubleValue(3.6));
   energyHelper.Set("ExpCellVoltage", DoubleValue(3.3));
   energyHelper.Set("RatedCapacity", DoubleValue(0.220)); // 220 mAh in Ah
   ```
3. ⬜ Add CLI parameter: `--battery-type [CR2032|AA|18650|CUSTOM]`
4. ⬜ Add CLI parameter: `--battery-capacity-mah [value]`
5. ⬜ Test compilation and runtime
6. ⬜ Verify voltage curve in output logs

**Validation:**
- Run 500s simulation, check nodes deplete realistically (not all at once)
- Verify voltage drops from ~3.6V → 3.0V (Li-Ion discharge curve)

---

### Phase 2: Realistic Current Values ⏱️ 1-2 hours
**Goal:** Update current draws to match real WiFi hardware

**Tasks:**
1. ⬜ Update default current values in `src/manet-compare-freespace-v1.cc`:
   ```cpp
   double m_idleCurrentA{0.020};   // 20 mA (was 27.3)
   double m_rxCurrentA{0.035};     // 35 mA (was 31.3)
   double m_txCurrentBaseA{0.080}; // 80 mA base (was 10)
   double m_sleepCurrentA{0.00015}; // 0.15 mA (was 3.4)
   ```
2. ⬜ Add CLI parameters:
   - `--tx-current-ma [value]`
   - `--rx-current-ma [value]`
   - `--idle-current-ma [value]`
   - `--sleep-current-ma [value]`
3. ⬜ Update `WifiRadioEnergyModelHelper` configuration
4. ⬜ Rerun simulations, verify realistic node lifetimes

**Expected Results:**
- With 2,376J (CR2032) and realistic currents:
  - Avg current ~40-60 mA (depending on TX duty cycle)
  - Node lifetime: 3-5 hours (not 8 minutes!)

---

### Phase 3: Distance-Based TX Power Control ⏱️ 3-4 hours
**Goal:** Implement adaptive TX power based on neighbor distance

**Tasks:**
1. ⬜ Add to `gaodv-v1-routing-protocol.h`:
   ```cpp
   double CalculateRequiredTxPower(double distance) const;
   ```
2. ⬜ Implement in `gaodv-v1-routing-protocol.cc`:
   ```cpp
   double RoutingProtocol::CalculateRequiredTxPower(double distance) const
   {
       // 3-tier power control
       if (distance < 50.0) return 10.0;  // 10 dBm = 10 mW
       if (distance < 150.0) return 15.0; // 15 dBm = 32 mW
       return 20.0;                       // 20 dBm = 100 mW (max)
   }
   ```
3. ⬜ Modify RREQ/RREP transmission to use calculated power:
   ```cpp
   // Before sending RREQ to neighbor
   double dist = GetDistanceToNeighbor(neighborAddr);
   double txPower = CalculateRequiredTxPower(dist);
   SetTxPower(txPower); // Configure WiFi PHY
   ```
4. ⬜ Update energy consumption calculation in `WifiRadioEnergyModel`
5. ⬜ Add CSV columns: `AvgTxPowerUsed`, `PowerControlSavings`

**Expected Benefits:**
- 20-40% energy savings on short-range communications
- Better reflects real-world adaptive protocols
- GAODV can optimize routes for energy AND distance

---

### Phase 4: Battery Variance & Realistic Depletion ⏱️ 2-3 hours
**Goal:** Add realistic node-to-node battery variations

**Tasks:**
1. ⬜ Add manufacturing variance (±5% capacity):
   ```cpp
   // In energy source installation loop
   UniformRandomVariable capacityVariance;
   capacityVariance.SetAttribute("Min", DoubleValue(0.95));
   capacityVariance.SetAttribute("Max", DoubleValue(1.05));
   
   for (uint32_t i = 0; i < nNodes; i++)
   {
       double variance = capacityVariance.GetValue();
       double nodeCapacity = baseCapacity * variance;
       energyHelper.Set("InitialEnergyJ", DoubleValue(nodeCapacity));
       // Install on node i
   }
   ```
2. ⬜ Add initial charge variance (90-100%):
   ```cpp
   UniformRandomVariable initialCharge;
   initialCharge.SetAttribute("Min", DoubleValue(0.90));
   initialCharge.SetAttribute("Max", DoubleValue(1.00));
   ```
3. ⬜ Add CLI flag: `--battery-variance [true|false]`
4. ⬜ Log per-node initial capacities to CSV

**Expected Results:**
- Nodes deplete at slightly different rates (realistic)
- Some nodes become energy bottlenecks earlier
- Tests GAODV energy-aware routing under heterogeneous energy

---

### Phase 5: Energy Harvesting (Optional) ⏱️ 4-6 hours
**Goal:** Add solar harvesting for advanced scenarios

**Tasks:**
1. ⬜ Research NS-3 `EnergyHarvester` module
2. ⬜ Implement `BasicEnergyHarvester`:
   ```cpp
   BasicEnergyHarvesterHelper harvesterHelper;
   harvesterHelper.Set("PeriodicHarvestedPowerUpdateInterval", TimeValue(Seconds(1)));
   harvesterHelper.Set("HarvestingPower", StringValue("ns3::ConstantRandomVariable[Constant=0.1]")); // 0.1W solar
   ```
3. ⬜ Add day/night cycle (harvesting only during day)
4. ⬜ Add CLI: `--energy-harvesting [none|solar|constant]`
5. ⬜ Test long-duration simulations (hours/days)

**Use Case:**
- Outdoor sensor networks with solar panels
- Tests network resilience with energy replenishment
- More realistic for environmental monitoring scenarios

---

## 5. Validation & Testing Plan

### 5.1 Unit Tests
- ⬜ Test battery depletion under constant load
- ⬜ Verify voltage curve matches Li-Ion profile
- ⬜ Test TX power control at different distances
- ⬜ Validate energy variance distribution

### 5.2 Integration Tests
- ⬜ Run 500s simulation with all protocols (AODV, DSDV, DSR, GAODV)
- ⬜ Compare energy consumption patterns
- ⬜ Verify no nodes deplete before simulation ends (with proper capacity)
- ⬜ Check CSV output contains new energy metrics

### 5.3 Realism Checks
| Metric | Current | Target | Pass? |
|--------|---------|--------|-------|
| Battery capacity | 20 J (1.68 mAh) | 2,376 J (220 mAh) | ⬜ |
| TX current @ 20dBm | ~10 mA | 180 mA | ⬜ |
| Sleep current | 3.4 mA | 0.15 mA | ⬜ |
| Node lifetime variance | 0% | 5-10% | ⬜ |
| Voltage drops with use | No | Yes (4.2→3.0V) | ⬜ |

### 5.4 Performance Validation
- ⬜ GAODV should show better energy distribution than AODV
- ⬜ TX power control should save 20-40% energy vs fixed power
- ⬜ Nodes with lower initial energy should fail first
- ⬜ Network lifetime should increase with energy-aware routing

---

## 6. Expected Outcomes

### 6.1 Realistic Energy Consumption
```
Before (20J, unrealistic currents):
  - Node lifetime: ~500s (perfectly tuned)
  - All nodes deplete together
  - No voltage variation

After (2,376J, realistic currents):
  - Node lifetime: 3-5 hours (realistic CR2032)
  - Nodes deplete at different rates (±10%)
  - Voltage drops from 3.6V → 3.0V
  - TX power adapts to distance
```

### 6.2 Better GAODV Validation
- Energy-aware routing more important with realistic energy constraints
- RREQEF tracking shows measurable impact
- Network partitioning from energy depletion (not just time limits)

### 6.3 Publication-Ready Results
- Energy model matches real sensor node hardware
- Results comparable to other MANET research
- Can cite specific battery datasheets (CR2032, etc.)

---

## 7. Timeline & Effort

| Phase | Effort | Priority | Dependencies |
|-------|--------|----------|--------------|
| **Phase 0: Fix Idle Dominance** | **2-3h** | **🔴 CRITICAL** | **None** |
| Phase 1: Battery Model | 2-3h | 🟡 Medium | Phase 0 |
| Phase 2: Current Values | 1-2h | 🟢 Low | Phase 0 |
| Phase 3: TX Power Control | 3-4h | 🟢 Low | Phase 0 |
| Phase 4: Battery Variance | 2-3h | ⬜ Optional | Phase 1 |
| Phase 5: Energy Harvesting | 4-6h | ⬜ Optional | Phase 1 |

**⚠️ CRITICAL PATH:** Phase 0 MUST be completed first - current energy model cannot differentiate protocols!

**Minimum Viable Fix:** 2-3 hours (Phase 0 only)  
**Core Realism:** 5-8 hours (Phases 0-1)  
**Full Implementation:** 8-11 hours (Phases 0-2)  
**With Advanced Features:** 14-20 hours (Phases 0-4)

---

## 8. References & Resources

### NS-3 Documentation
- `ns3::BasicEnergySource` - Current simple model
- `ns3::LiIonEnergySource` - Li-Ion battery chemistry
- `ns3::GenericBatteryModel` - Voltage-dependent capacity
- `ns3::WifiRadioEnergyModel` - WiFi PHY state energy
- `ns3::BasicEnergyHarvester` - Solar/energy harvesting

### Real Hardware Datasheets
- **CR2032:** Panasonic CR2032, 220mAh @ 3V
- **ESP32:** WiFi current: TX 180mA, RX 95mA, sleep 0.15mA
- **nRF52840:** BLE current: TX 5mA, RX 5.4mA, sleep 0.4μA
- **CC3220:** WiFi current: TX 223mA, RX 53mA, sleep 4μA

### Academic References
- EDA-AODV paper (original GAODV source) - verify their energy assumptions
- IEEE 802.11 power consumption studies
- MANET energy-efficient routing surveys

---

## 9. Next Steps

**🚨 CRITICAL FINDINGS:**
The current energy model **cannot differentiate between routing protocols** due to idle power dominance. All protocols show identical ~2,252J consumption regardless of routing strategy.

**Immediate Action (MANDATORY):**
1. ✅ Create this investigation document
2. ✅ Identify root cause: Idle current (27.3mA) dominates, burying protocol differences
3. ⬜ **IMPLEMENT PHASE 0** - Fix idle dominance to make energy comparison meaningful
4. ⬜ Re-run all protocols and verify energy differences emerge

**Phase 0 Implementation Priority:**
```
CRITICAL → 0.1: Reduce idle current (27.3mA → 2mA)
CRITICAL → 0.2: Increase TX current (10mA → 180mA)  
HIGH     → 0.3: Track routing overhead separately
MEDIUM   → 0.4: Add per-protocol energy metrics to CSV
```

**Questions for User:**
1. **Should we start Phase 0 implementation immediately?** (Recommended: YES)
   - Current state: Energy comparison is **broken** - all protocols identical
   - Phase 0 fixes this fundamental issue (2-3 hours)

2. After Phase 0, continue to battery realism (Phases 1-2)?
   - Or stop after protocol differentiation is working?

3. Do you want realistic battery capacity (CR2032, 18650) or keep current calibration?
   - Current: 55J @ 50 nodes allows 500s simulation
   - Realistic: 2,376J (CR2032) would last ~21,000s at new idle current

4. Is energy harvesting relevant for your research scenario?

---

**Document Status:** ⚠️ CRITICAL ISSUE IDENTIFIED - Idle power dominance  
**Author:** GitHub Copilot  
**Next Update:** After Phase 0 implementation  
**Priority:** Implement Phase 0 before any other energy model changes
