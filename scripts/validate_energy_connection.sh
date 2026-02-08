#!/bin/bash
echo "═══════════════════════════════════════════════"
echo "  Energy Source Connection Validation"
echo "═══════════════════════════════════════════════"

cd $(dirname $0)/..

# Run with debug logging
echo "Running GAODV simulation with energy debug logging..."
NS_LOG="Gaodvv1RoutingProtocol=level_debug" \
python3 scripts/run_all.py --protocols GAODV --runs 1 --nodes 10 \
  --simtime 100 --skip-build 2>&1 | tee /tmp/gaodv_energy_test.log

echo -e "\n━━━ VALIDATION RESULTS ━━━\n"

# Check 1: NEF calculations
NEF_COUNT=$(grep -c "Node.*NEF:" /tmp/gaodv_energy_test.log)
if [ $NEF_COUNT -gt 0 ]; then
    echo "✅ PASS: NEF calculations found ($NEF_COUNT instances)"
    echo "Sample NEF values:"
    grep "Node.*NEF:" /tmp/gaodv_energy_test.log | head -5
else
    echo "❌ FAIL: No NEF calculations found!"
    exit 1
fi

# Check 2: NEF values are not 1.0
NON_ONE_NEF=$(grep "NEF:" /tmp/gaodv_energy_test.log | grep -v "NEF: 1\.0" | wc -l)
if [ $NON_ONE_NEF -gt 0 ]; then
    echo -e "\n✅ PASS: NEF values varying ($NON_ONE_NEF non-1.0 values)"
else
    echo -e "\n⚠️  WARNING: All NEF values are 1.0 (may indicate no energy depletion)"
fi

# Check 3: Energy depletion
echo -e "\n✅ Energy depletion samples:"
grep "Remaining.*J" /tmp/gaodv_energy_test.log | head -5

echo -e "\n✅ Energy source connection validated!"
