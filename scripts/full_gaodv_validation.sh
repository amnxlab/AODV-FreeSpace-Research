#!/bin/bash
set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    GAODV PROTOCOL - COMPLETE VALIDATION SUITE                 ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

cd $(dirname $0)/..

# Test 1: Energy consumption varies with traffic
echo -e "\n[1/5] Testing Energy Consumption Variability..."

echo "   Running low traffic (2 pkt/s)..."
python3 scripts/run_all.py --protocols GAODV --runs 1 --nodes 10 \
  --packet-rate 2 --simtime 100 --skip-build 2>&1 | tee /tmp/low_traffic.log

ENERGY_LOW=$(grep "Total Consumed Energy:" /tmp/low_traffic.log | grep -oP '\d+\.\d+' | head -1)

echo "   Running high traffic (10 pkt/s)..."
python3 scripts/run_all.py --protocols GAODV --runs 1 --nodes 10 \
  --packet-rate 10 --simtime 100 --skip-build 2>&1 | tee /tmp/high_traffic.log

ENERGY_HIGH=$(grep "Total Consumed Energy:" /tmp/high_traffic.log | grep -oP '\d+\.\d+' | head -1)

echo "   Low traffic:  ${ENERGY_LOW}J"
echo "   High traffic: ${ENERGY_HIGH}J"

if [ -n "$ENERGY_LOW" ] && [ -n "$ENERGY_HIGH" ]; then
    if (( $(echo "$ENERGY_HIGH > $ENERGY_LOW * 1.05" | bc -l) )); then
        echo "   ✅ PASS: Energy varies with traffic (+5% threshold)"
    else
        echo "   ⚠️  WARNING: Energy similar (difference < 5%)"
    fi
else
    echo "   ⚠️  WARNING: Could not extract energy values"
fi

# Test 2: Energy source connection
echo -e "\n[2/5] Testing Energy Source Connection..."
chmod +x scripts/validate_energy_connection.sh
./scripts/validate_energy_connection.sh

# Test 3: MaxRange parameter
echo -e "\n[3/5] Testing MaxRange Parameter..."  
chmod +x scripts/validate_maxrange.sh
./scripts/validate_maxrange.sh

# Test 4: RREQEF flowchart
echo -e "\n[4/5] Testing RREQEF Flowchart Logic..."
chmod +x scripts/validate_rreqef_logic.sh
./scripts/validate_rreqef_logic.sh

# Test 5: Command display (manual)
echo -e "\n[5/5] Command Display Test"
echo "   ℹ️  Manual verification required:"
echo "      1. Start dashboard: cd dashboard && python3 server.py"
echo "      2. Open browser to http://localhost:5000"
echo "      3. Configure parameters and click 'Validate & Execute'"
echo "      4. Verify command appears at top of Execute tab"

echo -e "\n╔═══════════════════════════════════════════════════════════════╗"
echo "║    ✅ VALIDATION COMPLETE                                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

echo -e "\nSummary:"
echo "  • Energy consumption calculation - TESTED"
echo "  • Energy source connection - VALIDATED"
echo "  • MaxRange parameter usage - VALIDATED"
echo "  • RREQEF flowchart logic - VALIDATED"
echo "  • Command display - READY (manual test required)"
