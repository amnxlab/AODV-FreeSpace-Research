#!/usr/bin/env python3
"""
UWSN V2 - Comprehensive Plotting System
========================================
Generates vertical grouped bar charts for simulation metrics.
Features: separate axes, value labels on bars, flood protocol support.

Usage:
    python3 plot_comprehensive.py results.csv --output-dir ./plots/
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
import os
from pathlib import Path

# Constants
PACKET_SIZE_BYTES = 40
PACKET_SIZE_BITS = PACKET_SIZE_BYTES * 8

def calculate_metrics(df):
    """Calculates additional metrics required for plotting."""
    


    # 1A. Average Residual Energy per Node (Clarity improvement)
    if 'total_residual_energy' in df.columns and 'nodes' in df.columns:
        # Denominator: nodes + sinks(1) + sender(1). 
        # But 'nodes' column usually usually refers to relay nodes.
        # Let's approximate by 'nodes' column if total device count isn't explicit, 
        # or use (nodes + sinks + 1) if columns available.
        if 'sinks' in df.columns:
            total_devs = df['nodes'] + df['sinks'] + 1
        else:
            total_devs = df['nodes'] + 2 # Assume 1 sink 1 sender
        
        df['avg_residual_energy'] = df['total_residual_energy'] / total_devs

    # 2. Energy Efficiency (bits/Joule)
    # Formula: Energy Efficiency = (Successfully received bits) / (Total Energy Consumed)
    # Where: received bits = sink_recv × packet_size_bits
    if 'energy_efficiency_bits_per_joule' not in df.columns:
        if 'sink_recv' in df.columns and 'total_initial_energy' in df.columns and 'total_residual_energy' in df.columns:
            # Calculate received bits
            received_bits = df['sink_recv'] * PACKET_SIZE_BITS
            
            # Calculate consumed energy
            total_consumed = df['total_initial_energy'] - df['total_residual_energy']
            
            # Energy Efficiency = received_bits / consumed_energy
            df['energy_efficiency_bits_per_joule'] = received_bits / total_consumed.replace(0, np.nan)
            df['energy_efficiency_bits_per_joule'] = df['energy_efficiency_bits_per_joule'].fillna(0)
            
            print("Note: Calculated Energy Efficiency using formula: received_bits / consumed_energy (bits/J)")
    
    # Legacy support: if old column exists, rename it
    if 'energy_efficiency_percent' in df.columns and 'energy_efficiency_bits_per_joule' not in df.columns:
        print("Warning: Found old 'energy_efficiency_percent' column. Please re-run simulation for correct formula.")
    
    # 3. PDR Percentage
    if 'pdr' in df.columns:
        if df['pdr'].max() <= 1.0:
            df['pdr_percent'] = df['pdr'] * 100
        else:
            df['pdr_percent'] = df['pdr']
            
    # Ensure other metrics exist or fill with 0
    cols_to_check = ['avg_delay_ms', 'network_lifetime_s', 'total_residual_energy']
    for col in cols_to_check:
        if col not in df.columns:
            df[col] = 0
            
    return df


def generate_grouped_bar_plot(df, x_var, y_var, output_dir, result_filename_stem):
    """
    Generates a VERTICAL grouped bar chart.
    x_var: The independent variable (e.g., 'nodes'). categories on X-axis.
    y_var: The dependent variable (e.g., 'pdr_percent'). Height of bars.
    """
    
    # Labels - Clear, complete titles
    label_map = {
        'nodes': 'Number of Nodes',
        'maxDepth': 'Maximum Depth (m)',
        'vbfWidth': 'VBF Pipe Width (m)',
        'mobility': 'Mobility Model',
        'range': 'Communication Range (m)',
        'avg_residual_energy': 'Average Residual Energy per Node (J)',
        'energy_efficiency_bits_per_joule': 'Energy Efficiency (bits/J)',
        'pdr_percent': 'Packet Delivery Ratio - PDR (%)',
        'avg_delay_ms': 'Average End-to-End Delay (ms)',
        'network_lifetime_s': 'Network Lifetime (seconds)',
        'areaX': 'Simulation Area Width X (m)',
        'areaY': 'Simulation Area Length Y (m)',
    }
    
    xlabel = label_map.get(x_var, x_var)
    ylabel = label_map.get(y_var, y_var)
    
    # Prepare data
    if x_var not in df.columns:
        return
    
    # Filter: vbfWidth only applies to VBF and EVBF protocols (all variants)
    plot_df = df.copy()
    if x_var == 'vbfWidth':
        vbf_family = ['VBF', 'EVBF', 'EVBFv2', 'EVBFv3', 'EVBFv4']
        plot_df = df[df['protocol'].isin(vbf_family)]
        if plot_df.empty:
            return
        
    x_categories = sorted(plot_df[x_var].unique())
    protocols = sorted(plot_df['protocol'].unique())
    
    # Skip if no categories
    if not x_categories:
        return

    # Setup plot
    fig, ax = plt.subplots(figsize=(10, 6)) # Moderately sized
    
    # Dimensions
    n_groups = len(x_categories)
    n_bars = len(protocols)
    
    # Bar width and positions
    total_group_width = 0.8
    bar_width = total_group_width / n_bars
    indices = np.arange(n_groups)
    
    # Colors suitable for publications (CB-safe)
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']
    
    max_height = 0
    
    for i, protocol in enumerate(protocols):
        means = []
        stds = []
        
        for x_val in x_categories:
            subset = plot_df[(plot_df['protocol'] == protocol) & (plot_df[x_var] == x_val)]
            if y_var in subset.columns and not subset.empty:
                val = subset[y_var].mean()
                std = subset[y_var].std()
                if np.isnan(std): std = 0
                means.append(val)
                stds.append(std)
            else:
                means.append(0)
                stds.append(0)
        
        # Track max height for margins
        if means:
            # Check max of mean + std to account for error bars
            current_cluster_max = max([m + s for m, s in zip(means, stds)])
            if current_cluster_max > max_height:
                max_height = current_cluster_max
        
        # Position for this protocol's bars
        # Center the group on 'indices'
        # Group starts at indices - total_group_width/2
        # Bar i starts at: group_start + i*bar_width
        x_pos = indices - (total_group_width / 2) + (i * bar_width) + (bar_width / 2)
        
        # Draw bars
        bars = ax.bar(x_pos, means, width=bar_width, label=protocol, 
                      yerr=stds, capsize=3, color=colors[i % len(colors)], alpha=0.9, edgecolor='black', linewidth=0.7)
        
        # Add values on top of bars
        for bar, mean, std in zip(bars, means, stds):
            height = bar.get_height()
            if height > 0: # Only label if bar exists
                # Format: "Mean" (std is shown via error bar, adding text might clutter)
                # If user wants clear readable fonts, maybe just Mean is enough, or compact Mean±Std?
                # Let's do compact: "95.2" or "95.2\n±1.2" if space permits.
                # Given "font doesn't override", simple is better.
                # Let's stick to Mean, with Std in error bar.
                # label_text = f"{mean:.1f}" 
                # User asked for "Show results above each chart, write them, and write the error value as well" previously.
                # So we must include error in text? "write the error value as well".
                # Okay, use newline format but ensure small font.
                
                label_text = f"{mean:.1f}\n±{std:.1f}"
                
                y_pos = height + std + (max_height * 0.02) # Start above error bar
                
                ax.text(bar.get_x() + bar.get_width()/2., y_pos,
                        label_text,
                        ha='center', va='bottom', fontsize=8, fontweight='normal', color='black',
                        bbox=dict(facecolor='white', alpha=0.7, pad=0.5, edgecolor='none')) # Background to prevent overlap with grid lines

    # Styling
    ax.set_xticks(indices)
    
    # Format X labels (add units if needed)
    x_labels_formatted = []
    for val in x_categories:
        str_val = str(val)
        if x_var == 'range' or x_var == 'maxDepth' or x_var == 'vbfWidth':
             # Append 'm' if not present
             if not str_val.endswith('m'):
                 str_val += " m"
        x_labels_formatted.append(str_val)
        
    ax.set_xticklabels(x_labels_formatted, fontsize=11, fontweight='bold')
    
    ax.set_xlabel(xlabel, fontweight='bold', fontsize=12, labelpad=10)
    ax.set_ylabel(ylabel, fontweight='bold', fontsize=12, labelpad=10)
    ax.set_title(f"{xlabel} vs {ylabel}", fontweight='bold', fontsize=13, pad=15)
    
    # Legend
    ax.legend(title="Protocol", loc='best', fontsize=10, frameon=True, framealpha=0.9)
    
    # Grid
    ax.grid(axis='y', linestyle='--', alpha=0.4)
    ax.set_axisbelow(True) # Grid behind bars
    
    # Cleanup axes
    ax.set_ylim(bottom=0)
    # Increase top margin significantly to accommodate labels
    ax.set_ylim(top=max_height * 1.25) 
    
    # Descriptive filename mapping
    y_name_map = {
        'avg_residual_energy': 'Residual_Energy',
        'energy_efficiency_bits_per_joule': 'Energy_Efficiency',
        'pdr_percent': 'PDR',
        'avg_delay_ms': 'Average_Delay',
        'network_lifetime_s': 'Network_Lifetime'
    }
    x_name_map = {
        'nodes': 'Node_Count',
        'range': 'Range',
        'maxDepth': 'Depth',
        'vbfWidth': 'VBF_Width',
        'mobility': 'Mobility',
        'areaX': 'Area_Width',
        'areaY': 'Area_Length'
    }
    
    y_desc = y_name_map.get(y_var, y_var)
    x_desc = x_name_map.get(x_var, x_var)
    filename = f"{y_desc}_by_{x_desc}.png"
    output_path = os.path.join(output_dir, filename)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()
    print(f"  ✓ Saved: {filename}")


def main():
    parser = argparse.ArgumentParser(description="Generate plots for UWSN simulation.")
    parser.add_argument("csv_file", help="Path to results CSV")
    parser.add_argument("--output-dir", help="Directory to save plots")
    args = parser.parse_args()
    
    csv_file = args.csv_file
    output_dir = args.output_dir if args.output_dir else os.path.join(os.path.dirname(csv_file), "graphs")
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"Processing: {csv_file}")
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading CSV: {e}")
        sys.exit(1)
    
    # Calculate Metrics
    df = calculate_metrics(df)
    
    # Define Analysis Matrix
    # X Variables (Independent)
    x_vars = ['nodes', 'maxDepth', 'vbfWidth', 'mobility', 'range', 'areaX', 'areaY']
    
    # Y Variables (Dependent)
    y_vars = ['avg_residual_energy', 'energy_efficiency_bits_per_joule', 'pdr_percent', 
              'avg_delay_ms', 'network_lifetime_s']
    
    # Filter active X vars (must have variation)
    varying_x = [x for x in x_vars if x in df.columns and df[x].nunique() > 1]
    
    # Force 'nodes' if available and nothing else varies (common user request case)
    if not varying_x and 'nodes' in df.columns:
        varying_x = ['nodes']
    # If still nothing, fallback to first available parameter
    if not varying_x:
         varying_x = [x for x in x_vars if x in df.columns][:1]

    print(f"Plotting against Independent Variables (X-axis): {varying_x}")
    print(f"Output Directory: {output_dir}")
    
    result_filename_stem = Path(csv_file).stem
    
    count = 0
    for x_var in varying_x:
        for y_var in y_vars:
            if y_var in df.columns:
                generate_grouped_bar_plot(df, x_var, y_var, output_dir, result_filename_stem)
                count += 1
    
    # === Special Plot: Energy Consumption for All Protocols ===
    all_protocols = ['VBF', 'DBR', 'flooding', 'EDBR', 'EVBF', 'EDBRv2', 'EVBFv2', 
                     'EDBRv3', 'EVBFv3', 'EDBRv4', 'EVBFv4']
    df_with_protocols = df[df['protocol'].isin(all_protocols)]
    
    if not df_with_protocols.empty and 'nodes' in df_with_protocols.columns:
        # Calculate total consumed energy if not present
        if 'total_consumed_energy' not in df_with_protocols.columns:
            if 'total_initial_energy' in df_with_protocols.columns and 'total_residual_energy' in df_with_protocols.columns:
                df_with_protocols = df_with_protocols.copy()
                df_with_protocols['total_consumed_energy'] = df_with_protocols['total_initial_energy'] - df_with_protocols['total_residual_energy']
        
        if 'total_consumed_energy' in df_with_protocols.columns:
            # Generate the energy consumption plot for all protocols
            print(f"Generating Energy Consumption plot for all protocols...")
            
            # Group by nodes and protocol - calculate both mean and std
            grouped_mean = df_with_protocols.groupby(['nodes', 'protocol'])['total_consumed_energy'].mean().unstack()
            grouped_std = df_with_protocols.groupby(['nodes', 'protocol'])['total_consumed_energy'].std().unstack().fillna(0)
            
            if not grouped_mean.empty:
                x = np.arange(len(grouped_mean.index))
                n_protocols = len(grouped_mean.columns)
                width = 0.8 / n_protocols
                
                fig, ax = plt.subplots(figsize=(16, 8))
                
                colors = {
                    # Base Protocols - Distinct Primary Colors
                    'VBF': '#1565C0',      # Strong Blue
                    'DBR': '#2E7D32',      # Dark Green
                    'flooding': '#6A1B9A', # Deep Purple
                    # EDBR Family (Warm/Red Spectrum - well separated)
                    'EDBR': '#C62828',     # Dark Red
                    'EDBRv2': '#E65100',   # Bright Orange
                    'EDBRv3': '#F9A825',   # Golden Yellow
                    'EDBRv4': '#4E342E',   # Dark Brown
                    # EVBF Family (Cool Spectrum - well separated)
                    'EVBF': '#00838F',     # Dark Cyan
                    'EVBFv2': '#5E35B1',   # Deep Violet
                    'EVBFv3': '#00695C',   # Dark Teal
                    'EVBFv4': '#D81B60'    # Magenta/Pink
                }
                
                max_height = 0
                for i, protocol in enumerate(grouped_mean.columns):
                    offset = (i - n_protocols/2 + 0.5) * width
                    means = grouped_mean[protocol].values
                    stds = grouped_std[protocol].values
                    
                    # Track max height for label positioning
                    for m, s in zip(means, stds):
                        if not np.isnan(m) and (m + s) > max_height:
                            max_height = m + s
                    
                    # Draw bars with error bars
                    bars = ax.bar(x + offset, means, width, 
                                 label=protocol, color=colors.get(protocol, f'C{i}'),
                                 yerr=stds, capsize=2,
                                 edgecolor='white', linewidth=0.8)
                    
                    # Add value labels with mean±std
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
                ax.set_xticklabels(grouped_mean.index.astype(int))
                ax.legend(title='Protocol', loc='upper left')
                ax.grid(axis='y', alpha=0.3)
                ax.set_ylim(top=max_height * 1.25)  # Add space for labels
                
                plt.tight_layout()
                output_path = os.path.join(output_dir, 'Energy_Consumption_All_Protocols.png')
                plt.savefig(output_path, dpi=150, bbox_inches='tight', facecolor='white')
                plt.close()
                print(f"  Saved: {output_path}")
                count += 1
                
    print(f"\nGenerated {count} plots.")

if __name__ == "__main__":
    main()
