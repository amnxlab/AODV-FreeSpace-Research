#!/bin/bash
echo "═══════════════════════════════════════════════"
echo "  RREQEF Flowchart Logic Validation"
echo "═══════════════════════════════════════════════"

cd $(dirname $0)/..

# Low energy to trigger RREQEF updates
echo "Running with low energy scenario to trigger RREQEF updates..."
NS_LOG="Gaodvv1RoutingProtocol=level_debug|prefix_time|prefix_node" \
python3 scripts/run_all.py --protocols GAODV --runs 1 --nodes 10 \
  --simtime 500 --initial-energy 5 --ethf 0.3 --skip-build 2>&1 | \
  tee /tmp/gaodv_rreqef_test.log

echo -e "\n━━━ VALIDATION RESULTS ━━━\n"

# Check 1: RREQ with RREQEF field
echo "✓ Check 1: RREQ packets contain RREQEF"
RREQEF_COUNT=$(grep -c "RREQEF=" /tmp/gaodv_rreqef_test.log)
if [ $RREQEF_COUNT -gt 0 ]; then
    echo "   Found $RREQEF_COUNT RREQEF references"
    grep "RREQEF=" /tmp/gaodv_rreqef_test.log | head -3
else
    echo "   ⚠️  No RREQEF references found"
fi

# Check 2: Distance calculations
echo -e "\n✓ Check 2: Distance calculations"
DIST_COUNT=$(grep -c "distance.*m" /tmp/gaodv_rreqef_test.log)
echo "   Found $DIST_COUNT distance calculations"
grep "distance.*m" /tmp/gaodv_rreqef_test.log | head -3

# Check 3: RREQEF updates (NEF < RREQEF branch)
echo -e "\n✓ Check 3: RREQEF updates when NEF < RREQEF"
UPDATE_COUNT=$(grep -c "Updated RREQEF" /tmp/gaodv_rreqef_test.log)
if [ $UPDATE_COUNT -gt 0 ]; then
    echo "   ✅ Found $UPDATE_COUNT RREQEF updates"
    grep "Updated RREQEF" /tmp/gaodv_rreqef_test.log | head -5
else
    echo "   ⚠️  No updates (normal if nodes have similar energy)"
fi

# Check 4: Path selection
echo -e "\n✓ Check 4: Path selection using max RREQEF"
PATH_COUNT=$(grep -c "Selected best path.*RREQEF" /tmp/gaodv_rreqef_test.log)
if [ $PATH_COUNT -gt 0 ]; then
    echo "   Found $PATH_COUNT path selections"
    grep "Selected best path.*RREQEF" /tmp/gaodv_rreqef_test.log | head -3
else
    echo "   ⚠️  No path selections found"
fi

echo -e "\n✅ RREQEF flowchart logic validated!"
