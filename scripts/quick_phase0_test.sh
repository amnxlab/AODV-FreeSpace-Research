#!/bin/bash
# Quick Phase 0 validation test
# Tests that protocols show different energy consumption with Phase 0 model

echo "========================================="
echo "Phase 0 Energy Model Quick Test"
echo "========================================="
echo ""
echo "Testing: GAODV vs AODV (50 nodes, 100s)"
echo "Expected: GAODV ~50-60% less energy than AODV"
echo ""

cd /home/ahmed/AODV-FreeSpace-Research

# Run simulation
python3 scripts/run_all.py --protocols GAODV AODV --runs 1 --nodes 50 --simtime 100 --output-dir results

echo ""
echo "========================================="
echo "Test Complete!"
echo "========================================="
echo ""
echo "✓ If GAODV shows ~14-20J and AODV shows ~30-40J, Phase 0 is working"
echo "✓ Energy breakdown should show: Idle 97-99%, Routing 0.5-1.5%, Data 0.5-1.5%"
echo "✓ GAODV should use 50-60% less energy than AODV"
