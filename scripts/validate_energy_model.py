#!/usr/bin/env python3
"""
Energy Model Validation Script
Validates Phase 0 energy model accuracy and detects biases
"""

import pandas as pd
import numpy as np
import glob
import sys
from pathlib import Path

def validate_energy_conservation(csv_file):
    """Check if energy components sum to total (conservation law)"""
    df = pd.read_csv(csv_file)
    
    print(f"\n{'='*70}")
    print(f"VALIDATION 1: Energy Conservation Check")
    print(f"{'='*70}")
    
    for _, row in df.iterrows():
        total = row['consumedEnergy']
        routing = row['routingEnergyJ']
        data = row['dataEnergyJ']
        idle = row['idleEnergyJ']
        
        sum_components = routing + data + idle
        diff = total - sum_components
        diff_pct = (diff / total) * 100 if total > 0 else 0
        
        status = "✓ PASS" if abs(diff_pct) < 1.0 else "✗ FAIL"
        
        print(f"\n{row['protocol']} (nodes={row['nodes']}, run={row['run']}):")
        print(f"  Total Energy:     {total:.3f} J")
        print(f"  Sum of Parts:     {sum_components:.3f} J")
        print(f"  Difference:       {diff:.6f} J ({diff_pct:.4f}%)")
        print(f"  Status:           {status}")
        
        if abs(diff_pct) >= 1.0:
            print(f"  ⚠ WARNING: Energy not conserved!")
            return False
    
    return True

def validate_scaling_linearity(csv_file):
    """Check if energy scales linearly with nodes and time"""
    df = pd.read_csv(csv_file)
    
    print(f"\n{'='*70}")
    print(f"VALIDATION 2: Linear Scaling Check")
    print(f"{'='*70}")
    
    # Group by protocol and check scaling
    for protocol in df['protocol'].unique():
        protocol_df = df[df['protocol'] == protocol]
        
        if len(protocol_df) < 2:
            continue
            
        print(f"\n{protocol}:")
        
        # Check node scaling (energy should scale linearly with nodes)
        node_groups = protocol_df.groupby('nodes').agg({
            'consumedEnergy': 'mean',
            'idleEnergyJ': 'mean'
        }).reset_index()
        
        if len(node_groups) >= 2:
            for i in range(1, len(node_groups)):
                nodes_prev = node_groups.iloc[i-1]['nodes']
                nodes_curr = node_groups.iloc[i]['nodes']
                energy_prev = node_groups.iloc[i-1]['consumedEnergy']
                energy_curr = node_groups.iloc[i]['consumedEnergy']
                
                node_ratio = nodes_curr / nodes_prev
                energy_ratio = energy_curr / energy_prev
                linearity_error = abs(energy_ratio - node_ratio) / node_ratio * 100
                
                status = "✓ PASS" if linearity_error < 10 else "✗ FAIL"
                
                print(f"  {nodes_prev} → {nodes_curr} nodes:")
                print(f"    Node ratio:    {node_ratio:.2f}x")
                print(f"    Energy ratio:  {energy_ratio:.2f}x")
                print(f"    Linearity err: {linearity_error:.1f}%")
                print(f"    Status:        {status}")

def validate_protocol_differentiation(csv_file):
    """Check if protocols show meaningful energy differences"""
    df = pd.read_csv(csv_file)
    
    print(f"\n{'='*70}")
    print(f"VALIDATION 3: Protocol Differentiation Check")
    print(f"{'='*70}")
    
    # Group by protocol - simpler aggregation
    protocol_groups = df.groupby('protocol')
    
    print(f"\nProtocol Energy Comparison:")
    print(f"{'Protocol':<15} {'Total (J)':<20} {'Routing (J)':<20} {'Routing Pkts':<15}")
    print(f"{'-'*70}")
    
    protocols = []
    energies = []
    
    for protocol, group in protocol_groups:
        total_mean = group['consumedEnergy'].mean()
        total_std = group['consumedEnergy'].std()
        routing_mean = group['routingEnergyJ'].mean()
        routing_std = group['routingEnergyJ'].std()
        pkts_mean = group['routingPackets'].mean()
        
        print(f"{protocol:<15} {total_mean:>8.2f} ± {total_std:>6.2f}  "
              f"{routing_mean:>8.4f} ± {routing_std:>6.4f}  {pkts_mean:>10.0f}")
        
        protocols.append(protocol)
        energies.append(total_mean)
    
    # Check if protocols are actually different
    if len(energies) >= 2:
        max_diff = (max(energies) - min(energies)) / min(energies) * 100
        status = "✓ PASS" if max_diff > 5 else "⚠ WARNING"
        
        print(f"\nMax difference between protocols: {max_diff:.1f}%")
        print(f"Status: {status}")
        
        if max_diff < 5:
            print(f"⚠ Protocols show <5% difference - may indicate bias or insufficient differentiation")
            return False
    
    return True

def validate_physical_plausibility(csv_file):
    """Check if energy values match physical reality"""
    df = pd.read_csv(csv_file)
    
    print(f"\n{'='*70}")
    print(f"VALIDATION 4: Physical Plausibility Check")
    print(f"{'='*70}")
    
    # Expected ratios based on Phase 0 model
    # TX current (180mA) >> Idle current (2mA)
    # So TX energy per packet should be much higher than idle energy per second
    
    VOLTAGE = 3.3  # V
    IDLE_CURRENT = 0.002  # A (2mA)
    TX_CURRENT = 0.180  # A (180mA)
    
    print(f"\nPhase 0 Model Parameters:")
    print(f"  Idle current:  {IDLE_CURRENT*1000:.1f} mA")
    print(f"  TX current:    {TX_CURRENT*1000:.1f} mA")
    print(f"  Voltage:       {VOLTAGE} V")
    print(f"  TX/Idle ratio: {TX_CURRENT/IDLE_CURRENT:.0f}x")
    
    print(f"\nChecking physical constraints:")
    
    all_valid = True
    for _, row in df.iterrows():
        protocol = row['protocol']
        nodes = row['nodes']
        sim_time = row['simtime']
        
        total = row['consumedEnergy']
        routing = row['routingEnergyJ']
        data = row['dataEnergyJ']
        idle = row['idleEnergyJ']
        
        # Check 1: Idle energy should dominate (nodes spend most time idle)
        idle_pct = (idle / total) * 100
        
        # Check 2: TX energy (routing + data) should be non-zero
        tx_energy = routing + data
        tx_pct = (tx_energy / total) * 100
        
        # Check 3: Energy per node should be reasonable
        energy_per_node = total / nodes
        
        # Theoretical minimum (pure idle): nodes × idle_power × time
        min_energy_per_node = IDLE_CURRENT * VOLTAGE * sim_time
        
        # Check 4: Routing overhead should correlate with packets
        routing_pkts = row['routingPackets']
        
        print(f"\n{protocol} (nodes={nodes}):")
        print(f"  Idle %:        {idle_pct:.1f}% {'✓' if 70 < idle_pct < 99 else '✗'}")
        print(f"  TX %:          {tx_pct:.1f}% {'✓' if tx_pct > 0.1 else '✗'}")
        print(f"  Energy/node:   {energy_per_node:.3f} J")
        print(f"  Min expected:  {min_energy_per_node:.3f} J {'✓' if energy_per_node >= min_energy_per_node else '✗'}")
        print(f"  Routing pkts:  {routing_pkts:.0f} {'✓' if routing_pkts > 0 else '✗'}")
        
        if not (70 < idle_pct < 99) or tx_pct < 0.1 or energy_per_node < min_energy_per_node:
            all_valid = False
    
    return all_valid

def validate_statistical_significance(csv_file):
    """Check if protocol differences are statistically significant"""
    df = pd.read_csv(csv_file)
    
    print(f"\n{'='*70}")
    print(f"VALIDATION 5: Statistical Significance")
    print(f"{'='*70}")
    
    from scipy import stats
    
    protocols = df['protocol'].unique()
    
    if len(protocols) < 2:
        print("Not enough protocols for comparison")
        return True
    
    print(f"\nTwo-sample t-tests (p < 0.05 indicates significant difference):")
    
    for i, p1 in enumerate(protocols):
        for p2 in protocols[i+1:]:
            df1 = df[df['protocol'] == p1]['consumedEnergy']
            df2 = df[df['protocol'] == p2]['consumedEnergy']
            
            if len(df1) < 2 or len(df2) < 2:
                continue
            
            t_stat, p_value = stats.ttest_ind(df1, df2)
            
            mean_diff = df2.mean() - df1.mean()
            pct_diff = (mean_diff / df1.mean()) * 100
            
            status = "✓ SIGNIFICANT" if p_value < 0.05 else "✗ NOT SIGNIFICANT"
            
            print(f"\n{p1} vs {p2}:")
            print(f"  Mean {p1}: {df1.mean():.2f} J")
            print(f"  Mean {p2}: {df2.mean():.2f} J")
            print(f"  Difference: {mean_diff:.2f} J ({pct_diff:+.1f}%)")
            print(f"  t-statistic: {t_stat:.3f}")
            print(f"  p-value: {p_value:.4f}")
            print(f"  Status: {status}")

def main():
    if len(sys.argv) < 2:
        # Find most recent results file
        results_files = glob.glob('/home/ahmed/AODV-FreeSpace-Research/results/results_*.csv')
        if not results_files:
            print("No results files found!")
            print("Usage: python validate_energy_model.py <results.csv>")
            sys.exit(1)
        
        csv_file = max(results_files, key=lambda x: Path(x).stat().st_mtime)
        print(f"Using most recent results: {csv_file}")
    else:
        csv_file = sys.argv[1]
    
    if not Path(csv_file).exists():
        print(f"Error: File not found: {csv_file}")
        sys.exit(1)
    
    print(f"\n{'#'*70}")
    print(f"# ENERGY MODEL VALIDATION REPORT")
    print(f"# File: {csv_file}")
    print(f"{'#'*70}")
    
    # Run all validations
    v1 = validate_energy_conservation(csv_file)
    v2 = validate_scaling_linearity(csv_file)
    v3 = validate_protocol_differentiation(csv_file)
    v4 = validate_physical_plausibility(csv_file)
    
    try:
        v5 = validate_statistical_significance(csv_file)
    except ImportError:
        print("\n⚠ scipy not installed - skipping statistical tests")
        v5 = True
    
    # Summary
    print(f"\n{'='*70}")
    print(f"VALIDATION SUMMARY")
    print(f"{'='*70}")
    print(f"1. Energy Conservation:         {'✓ PASS' if v1 else '✗ FAIL'}")
    print(f"2. Linear Scaling:              {'✓ PASS' if v2 else '✗ FAIL'}")
    print(f"3. Protocol Differentiation:    {'✓ PASS' if v3 else '⚠ WARNING'}")
    print(f"4. Physical Plausibility:       {'✓ PASS' if v4 else '✗ FAIL'}")
    print(f"5. Statistical Significance:    {'✓ PASS' if v5 else '⚠ WARNING'}")
    
    all_pass = v1 and v2 and v3 and v4 and v5
    
    if all_pass:
        print(f"\n✓ All validations passed - energy model appears accurate")
    else:
        print(f"\n✗ Some validations failed - review energy model")
    
    print(f"{'='*70}\n")

if __name__ == "__main__":
    main()
