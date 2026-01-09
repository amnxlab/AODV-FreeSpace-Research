#!/usr/bin/env python3
"""
Plot Results - MANET Protocol Comparison
=========================================
Generates comparison plots from aggregated simulation results.

Usage:
    python3 plot_results.py
    python3 plot_results.py --input results/aggregated_results.csv --output graphs/
"""

import argparse
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys

# Configuration
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
DEFAULT_INPUT = PROJECT_DIR / "results" / "aggregated_results.csv"
DEFAULT_OUTPUT = PROJECT_DIR / "graphs"


def load_data(input_file):
    """Load and validate CSV data."""
    try:
        df = pd.read_csv(input_file)
        print(f"Loaded {len(df)} rows from {input_file}\n")
        return df
    except FileNotFoundError:
        print(f"ERROR: File not found: {input_file}")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: Could not load {input_file}: {e}")
        sys.exit(1)


def plot_pdr_comparison(df, output_dir):
    """Plot PDR comparison across protocols (NS3-Research style)."""
    fig, ax = plt.subplots(figsize=(12, 7))
    
    # Group by nodes and protocol
    if 'nodes' not in df.columns:
        df['nodes'] = 50  # Default if not present
    
    node_counts = sorted(df['nodes'].unique())
    protocols = sorted(df['protocol'].unique())
    
    # Calculate grouped statistics
    grouped_mean = df.groupby(['nodes', 'protocol'])['pdr'].mean().unstack() * 100
    grouped_std = df.groupby(['nodes', 'protocol'])['pdr'].std().unstack().fillna(0) * 100
    
    # Colors for protocols
    colors = {
        'GAODV': '#2196F3',    # Blue
        'AODV': '#4CAF50',     # Green  
        'OLSR': '#FF5722',     # Deep Orange
        'DSDV': '#9C27B0',     # Purple
        'DSR': '#FFC107'       # Amber
    }
    
    # Bar positions
    x = np.arange(len(node_counts))
    n_protocols = len(protocols)
    width = 0.8 / n_protocols
    
    max_height = 0
    
    for i, protocol in enumerate(protocols):
        if protocol not in grouped_mean.columns:
            continue
            
        offset = (i - n_protocols/2 + 0.5) * width
        means = grouped_mean[protocol].values
        stds = grouped_std[protocol].values
        
        # Track max height for labels
        for m, s in zip(means, stds):
            if not np.isnan(m) and (m + s) > max_height:
                max_height = m + s
        
        # Draw bars with error bars
        bars = ax.bar(x + offset, means, width,
                     label=protocol, color=colors.get(protocol, f'C{i}'),
                     yerr=stds, capsize=3,
                     edgecolor='white', linewidth=0.8)
        
        # Add value labels
        for bar, mean, std in zip(bars, means, stds):
            height = bar.get_height()
            if height > 0:
                label_text = f'{mean:.1f}%\n±{std:.1f}'
                y_pos = height + std + 1
                ax.annotate(label_text,
                           xy=(bar.get_x() + bar.get_width() / 2, y_pos),
                           ha='center', va='bottom', fontsize=8,
                           bbox=dict(facecolor='white', alpha=0.7, pad=0.3, edgecolor='none'))
    
    ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
    ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    ax.set_title('PDR Comparison Across Protocols', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(node_counts)
    ax.legend(title='Protocol', loc='best')
    ax.grid(axis='y', alpha=0.3)
    ax.set_ylim(0, min(100, max_height * 1.3))
    
    plt.tight_layout()
    output_file = output_dir / "pdr_comparison.png"
    plt.savefig(output_file, dpi=150, bbox_inches='tight', facecolor='white')
    print(f"✓ Saved: {output_file}")
    plt.close()


def plot_energy_comparison(df, output_dir):
    """Plot energy consumption comparison (NS3-Research style)."""
    fig, ax = plt.subplots(figsize=(14, 8))
    
    # Ensure nodes column exists
    if 'nodes' not in df.columns:
        df['nodes'] = 50
    
    node_counts = sorted(df['nodes'].unique())
    protocols = sorted(df['protocol'].unique())
    
    # Group by nodes and protocol
    grouped_mean = df.groupby(['nodes', 'protocol'])['consumedEnergy'].mean().unstack()
    grouped_std = df.groupby(['nodes', 'protocol'])['consumedEnergy'].std().unstack().fillna(0)
    
    # Colors matching NS3-Research
    colors = {
        'GAODV': '#2196F3',
        'AODV': '#4CAF50',
        'OLSR': '#FF5722',
        'DSDV': '#9C27B0',
        'DSR': '#FFC107'
    }
    
    # Bar setup
    x = np.arange(len(node_counts))
    n_protocols = len(protocols)
    width = 0.8 / n_protocols
    
    max_height = 0
    
    for i, protocol in enumerate(protocols):
        if protocol not in grouped_mean.columns:
            continue
            
        offset = (i - n_protocols/2 + 0.5) * width
        means = grouped_mean[protocol].values
        stds = grouped_std[protocol].values
        
        # Track max for labels
        for m, s in zip(means, stds):
            if not np.isnan(m) and (m + s) > max_height:
                max_height = m + s
        
        # Draw bars
        bars = ax.bar(x + offset, means, width,
                     label=protocol, color=colors.get(protocol, f'C{i}'),
                     yerr=stds, capsize=2,
                     edgecolor='white', linewidth=0.8)
        
        # Add value labels
        for bar, mean, std in zip(bars, means, stds):
            height = bar.get_height()
            if height > 0:
                label_text = f'{mean:.0f}\n±{std:.0f}'
                y_pos = height + std + (max_height * 0.01)
                ax.annotate(label_text,
                           xy=(bar.get_x() + bar.get_width() / 2, y_pos),
                           ha='center', va='bottom', fontsize=7,
                           bbox=dict(facecolor='white', alpha=0.7, pad=0.3, edgecolor='none'))
    
    ax.set_xlabel('Number of Nodes', fontsize=12, fontweight='bold')
    ax.set_ylabel('Total Energy Consumed (Joules)', fontsize=12, fontweight='bold')
    ax.set_title('Energy Consumption vs Node Count (All Protocols)', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(node_counts)
    ax.legend(title='Protocol', loc='upper left')
    ax.grid(axis='y', alpha=0.3)
    if max_height > 0:
        ax.set_ylim(top=max_height * 1.25)
    
    plt.tight_layout()
    output_file = output_dir / "energy_comparison.png"
    plt.savefig(output_file, dpi=150, bbox_inches='tight', facecolor='white')
    print(f"✓ Saved: {output_file}")
    plt.close()


def plot_efficiency_comparison(df, output_dir):
    """Plot energy efficiency (PDR/Energy) comparison."""
    plt.figure(figsize=(10, 6))
    
    # Calculate efficiency metric (PDR per unit energy)
    # Avoid division by zero
    df['efficiency'] = df.apply(
        lambda row: (row['pdr'] * 100) / row['consumedEnergy'] 
        if row['consumedEnergy'] > 0 else 0, 
        axis=1
    )
    
    grouped = df.groupby('protocol')['efficiency'].agg(['mean', 'std']).reset_index()
    
    # Check if all values are zero
    if grouped['mean'].sum() == 0:
        print("⚠ Skipping efficiency plot - all energy consumption values are zero")
        return
    
    protocols = grouped['protocol']
    efficiency_means = grouped['mean']
    efficiency_stds = grouped['std'].fillna(0)
    
    x = np.arange(len(protocols))
    bars = plt.bar(x, efficiency_means, yerr=efficiency_stds, capsize=5,
                   color=['#9b59b6', '#3498db', '#2ecc71', '#f39c12', '#e74c3c'][:len(protocols)],
                   alpha=0.8, edgecolor='black')
    
    plt.xlabel('Routing Protocol', fontsize=12, fontweight='bold')
    plt.ylabel('Energy Efficiency (PDR/kJ)', fontsize=12, fontweight='bold')
    plt.title('Energy Efficiency Comparison', fontsize=14, fontweight='bold')
    plt.xticks(x, protocols)
    plt.grid(axis='y', alpha=0.3, linestyle='--')
    
    # Add value labels
    for i, (mean, std) in enumerate(zip(efficiency_means, efficiency_stds)):
        plt.text(i, mean + std + max(efficiency_means)*0.02, f'{mean:.3f}', 
                ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout()
    output_file = output_dir / "efficiency_comparison.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file}")
    plt.close()


def plot_scatter_tradeoff(df, output_dir):
    """Plot PDR vs Energy trade-off scatter."""
    plt.figure(figsize=(10, 7))
    
    protocols = sorted(df['protocol'].unique())
    colors = ['#2ecc71', '#3498db', '#e74c3c', '#f39c12', '#9b59b6']
    
    for i, protocol in enumerate(protocols):
        data = df[df['protocol'] == protocol]
        plt.scatter(data['consumedEnergy'], data['pdr'] * 100, 
                   label=protocol, s=100, alpha=0.6, 
                   color=colors[i % len(colors)], edgecolors='black')
    
    plt.xlabel('Energy Consumed (J)', fontsize=12, fontweight='bold')
    plt.ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
    plt.title('PDR vs Energy Trade-off', fontsize=14, fontweight='bold')
    plt.legend(loc='best', frameon=True, shadow=True)
    plt.grid(alpha=0.3, linestyle='--')
    
    plt.tight_layout()
    output_file = output_dir / "tradeoff_scatter.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Saved: {output_file}")
    plt.close()


def generate_statistics_table(df, output_dir):
    """Generate and save statistics table."""
    protocols = sorted(df['protocol'].unique())
    
    stats_data = []
    for protocol in protocols:
        data = df[df['protocol'] == protocol]
        stats_data.append({
            'Protocol': protocol,
            'Runs': len(data),
            'PDR Mean (%)': f"{data['pdr'].mean() * 100:.2f}",
            'PDR Std (%)': f"{data['pdr'].std() * 100:.2f}",
            'Energy Mean (J)': f"{data['consumedEnergy'].mean():.2f}",
            'Energy Std (J)': f"{data['consumedEnergy'].std():.2f}",
            'Packets Sent (avg)': f"{data['packetsSent'].mean():.0f}",
            'Packets Recv (avg)': f"{data['packetsReceived'].mean():.0f}"
        })
    
    stats_df = pd.DataFrame(stats_data)
    stats_file = output_dir / "statistics.csv"
    stats_df.to_csv(stats_file, index=False)
    print(f"✓ Saved: {stats_file}")
    
    # Print to console
    print("\n" + "="*80)
    print("STATISTICS SUMMARY")
    print("="*80)
    print(stats_df.to_string(index=False))
    print("="*80 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="Generate comparison plots from simulation results")
    
    parser.add_argument('--input', type=Path, default=DEFAULT_INPUT,
                       help='Input CSV file with aggregated results')
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT,
                       help='Output directory for plots')
    
    args = parser.parse_args()
    
    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)
    
    print(f"\n{'='*70}")
    print(f"GENERATING PLOTS")
    print(f"{'='*70}\n")
    print(f"Input: {args.input}")
    print(f"Output: {args.output}\n")
    
    # Load data
    df = load_data(args.input)
    
    # Generate plots
    plot_pdr_comparison(df, args.output)
    plot_energy_comparison(df, args.output)
    plot_efficiency_comparison(df, args.output)
    plot_scatter_tradeoff(df, args.output)
    generate_statistics_table(df, args.output)
    
    print(f"\n{'='*70}")
    print(f"DONE - All plots generated successfully!")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
