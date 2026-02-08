#!/bin/bash
echo "═══════════════════════════════════════════════"
echo "  MaxRange Parameter Validation"
echo "═══════════════════════════════════════════════"

cd $(dirname $0)/..

# Use restrictive MaxRange
echo "Running with restrictive MaxRange (100m) and wide node spacing (150m)..."
NS_LOG="Gaodvv1RoutingProtocol=level_debug" \
python3 scripts/run_all.py --protocols GAODV --runs 1 --nodes 10 \
  --node-spacing 150 --max-range 100 --skip-build 2>&1 | \
  tee /tmp/gaodv_maxrange_test.log

echo -e "\n━━━ VALIDATION RESULTS ━━━\n"

# Check distance filtering
FILTER_COUNT=$(grep -c "exceeds MaxRange\|within MaxRange" /tmp/gaodv_maxrange_test.log)
if [ $FILTER_COUNT -gt 0 ]; then
    echo "✅ PASS: MaxRange filtering active ($FILTER_COUNT instances)"
    echo "Sample distance checks:"
    grep "MaxRange" /tmp/gaodv_maxrange_test.log | head -10
else
    echo "❌ FAIL: MaxRange not being used!"
    exit 1
fi

echo -e "\n✅ MaxRange parameter validated!"
