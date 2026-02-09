#!/usr/bin/env python3
"""
GAODV Dashboard - Enhanced Visualization Module
================================================
Adapted from UWSN plot_comprehensive.py for MANET protocol comparison.
Features: grouped bar charts, energy breakdown, publication-quality output.

Supports: GAODV, AODV, DSDV, DSR protocols
Energy Model: Phase 0 (routing/data/idle breakdown)
"""

import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
import numpy as np
import io
import base64
from typing import Dict, List, Optional, Any

# Protocol color scheme (color-blind safe)
PROTOCOL_COLORS = {
    'GAODV': '#1565C0',    # Strong Blue (energy-aware primary)
    'AODV':  '#C62828',    # Dark Red (baseline)
    'DSDV':  '#2E7D32',    # Dark Green (proactive)
    'DSR':   '#F9A825',    # Golden Yellow (source routing)
}

# Energy component colors (Phase 0 model)
ENERGY_COMPONENT_COLORS = {
    'routing': '#C62828',  # Dark Red (overhead)
    'data':    '#1565C0',  # Strong Blue (useful work)
    'idle':    '#757575',  # Gray (waste)
}

# Chart configuration
CHART_CONFIG = {
    'figsize': (12, 7),
    'dpi': 300,  # Publication quality
    'title_fontsize': 14,
    'label_fontsize': 12,
    'tick_fontsize': 11,
    'legend_fontsize': 11,
    'grid_alpha': 0.4,
    'grid_linestyle': '--',
}


def calculate_metrics(df: pd.DataFrame) -> pd.DataFrame:
    """
    Calculate additional metrics for plotting
    Adapted from plot_comprehensive.py for GAODV protocols
    """
    
    # 1. PDR percentage (if normalized to 0-1)
    if 'pdr' in df.columns and 'pdr_percent' not in df.columns:
        if df['pdr'].max() <= 1.0:
            df['pdr_percent'] = df['pdr'] * 100
        else:
            df['pdr_percent'] = df['pdr']
    
    # 2. Energy Efficiency (PDR per Joule - simpler than bits/J for protocol comparison)
    if 'consumedEnergy' in df.columns and 'pdr_percent' in df.columns:
        consumed = df['consumedEnergy'].replace(0, np.nan)
        df['energy_efficiency'] = df['pdr_percent'] / consumed
        df['energy_efficiency'] = df['energy_efficiency'].fillna(0)
    
    # 3. Routing overhead percentage
    if 'routingPackets' in df.columns and 'dataPackets' in df.columns:
        total_packets = df['routingPackets'] + df['dataPackets']
        total_packets = total_packets.replace(0, np.nan)
        df['routing_overhead_pct'] = (df['routingPackets'] / total_packets) * 100
        df['routing_overhead_pct'] = df['routing_overhead_pct'].fillna(0)
    
    # 4. Average residual energy per node
    if 'residualEnergy' in df.columns and 'nodes' in df.columns:
        df['avg_residual_energy_per_node'] = df['residualEnergy'] / df['nodes']
    
    # 5. Throughput (if not already present)
    if 'throughput' not in df.columns and 'packetsReceived' in df.columns and 'simtime' in df.columns:
        # Simple throughput: packets received per second
        df['throughput'] = df['packetsReceived'] / df['simtime'].replace(0, np.nan)
        df['throughput'] = df['throughput'].fillna(0)
    
    return df


def generate_grouped_bar_plot(df: pd.DataFrame, x_var: str, y_var: str,
                               title: str = None) -> str:
    """
    Generate vertical grouped bar chart with error bars
    Adapted from plot_comprehensive.py for GAODV protocols
    
    Args:
        df: DataFrame with simulation results
        x_var: Independent variable (X-axis categories)
        y_var: Dependent variable (Y-axis values)
        title: Custom title (optional)
        
    Returns:
        Base64 encoded PNG image
    """
    
    # Label mapping for axis titles
    label_map = {
        'nodes': 'Number of Nodes',
        'txPower': 'TX Power (dBm)',
        'simtime': 'Simulation Time (s)',
        'mobilityModel': 'Mobility Model',
        'nodeSpacing': 'Node Spacing (m)',
        'pdr_percent': 'Packet Delivery Ratio (%)',
        'consumedEnergy': 'Energy Consumed (J)',
        'residualEnergy': 'Residual Energy (J)',
        'routingPackets': 'Routing Packets',
        'dataPackets': 'Data Packets',
        'energy_efficiency': 'Energy Efficiency (PDR/J)',
        'routing_overhead_pct': 'Routing Overhead (%)',
        'avg_residual_energy_per_node': 'Avg Residual Energy per Node (J)',
        'throughput': 'Throughput (packets/s)',
    }
    
    xlabel = label_map.get(x_var, x_var)
    ylabel = label_map.get(y_var, y_var)
    
    # Validate columns exist
    if x_var not in df.columns or y_var not in df.columns:
        return None
    
    # Get unique categories and protocols
    x_categories = sorted(df[x_var].unique())
    protocols = sorted(df['protocol'].unique())
    
    if not x_categories or not protocols:
        return None
    
    # Setup plot
    fig, ax = plt.subplots(figsize=CHART_CONFIG['figsize'])
    
    n_groups = len(x_categories)
    n_bars = len(protocols)
    total_group_width = 0.8
    bar_width = total_group_width / n_bars
    indices = np.arange(n_groups)
    
    max_height = 0
    
    # Plot each protocol
    for i, protocol in enumerate(protocols):
        means = []
        stds = []
        
        # Calculate mean and std for each X category
        for x_val in x_categories:
            subset = df[(df['protocol'] == protocol) & (df[x_var] == x_val)]
            if not subset.empty and y_var in subset.columns:
                mean = subset[y_var].mean()
                std = subset[y_var].std()
                if np.isnan(std):
                    std = 0
                means.append(mean)
                stds.append(std)
            else:
                means.append(0)
                stds.append(0)
        
        # Track max height for label positioning
        if means:
            current_max = max([m + s for m, s in zip(means, stds) if not np.isnan(m)])
            if current_max > max_height:
                max_height = current_max
        
        # Calculate bar positions
        x_pos = indices - (total_group_width / 2) + (i * bar_width) + (bar_width / 2)
        
        # Get color
        color = PROTOCOL_COLORS.get(protocol, f'C{i}')
        
        # Draw bars with error bars
        bars = ax.bar(x_pos, means, width=bar_width, label=protocol,
                     yerr=stds, capsize=3, color=color, alpha=0.9,
                     edgecolor='black', linewidth=0.7)
        
        # Add value labels on top of bars
        for bar, mean, std in zip(bars, means, stds):
            height = bar.get_height()
            if height > 0:
                # Format: "mean±std"
                label_text = f"{mean:.1f}\n±{std:.1f}"
                y_pos = height + std + (max_height * 0.02)
                
                ax.text(bar.get_x() + bar.get_width()/2., y_pos,
                       label_text, ha='center', va='bottom', fontsize=8,
                       fontweight='normal', color='black',
                       bbox=dict(facecolor='white', alpha=0.7, pad=0.5, edgecolor='none'))
    
    # Styling (from reference)
    ax.set_xticks(indices)
    ax.set_xticklabels([str(x) for x in x_categories], 
                       fontsize=CHART_CONFIG['tick_fontsize'], fontweight='bold')
    ax.set_xlabel(xlabel, fontweight='bold', 
                  fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    ax.set_ylabel(ylabel, fontweight='bold', 
                  fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    
    plot_title = title if title else f"{ylabel} vs {xlabel}"
    ax.set_title(plot_title, fontweight='bold', 
                fontsize=CHART_CONFIG['title_fontsize'], pad=15)
    
    ax.legend(title="Protocol", loc='best', 
             fontsize=CHART_CONFIG['legend_fontsize'], frameon=True, framealpha=0.9)
    ax.grid(axis='y', linestyle=CHART_CONFIG['grid_linestyle'], 
           alpha=CHART_CONFIG['grid_alpha'])
    ax.set_axisbelow(True)  # Grid behind bars
    ax.set_ylim(bottom=0, top=max_height * 1.25 if max_height > 0 else 1)
    
    plt.tight_layout()
    
    # Convert to base64
    return fig_to_base64(fig)


def generate_energy_breakdown_stacked(df: pd.DataFrame) -> str:
    """
    Generate stacked bar chart showing energy breakdown by component
    Shows routing, data, and idle energy for each protocol (Phase 0 model)
    
    Args:
        df: DataFrame with routingEnergyJ, dataEnergyJ, idleEnergyJ columns
        
    Returns:
        Base64 encoded PNG image
    """
    
    if 'protocol' not in df.columns:
        return None
    
    required_cols = ['routingEnergyJ', 'dataEnergyJ', 'idleEnergyJ']
    if not all(col in df.columns for col in required_cols):
        return None
    
    protocols = sorted(df['protocol'].unique())
    fig, ax = plt.subplots(figsize=CHART_CONFIG['figsize'])
    
    # Calculate means for each component
    routing_means = []
    data_means = []
    idle_means = []
    
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        routing_means.append(proto_df['routingEnergyJ'].mean())
        data_means.append(proto_df['dataEnergyJ'].mean())
        idle_means.append(proto_df['idleEnergyJ'].mean())
    
    x = np.arange(len(protocols))
    width = 0.6
    
    # Create stacked bars
    p1 = ax.bar(x, routing_means, width, label='Routing',
               color=ENERGY_COMPONENT_COLORS['routing'], alpha=0.9, 
               edgecolor='black', linewidth=0.7)
    p2 = ax.bar(x, data_means, width, bottom=routing_means,
               label='Data', color=ENERGY_COMPONENT_COLORS['data'], alpha=0.9,
               edgecolor='black', linewidth=0.7)
    p3 = ax.bar(x, idle_means, width,
               bottom=[r+d for r,d in zip(routing_means, data_means)],
               label='Idle', color=ENERGY_COMPONENT_COLORS['idle'], alpha=0.9,
               edgecolor='black', linewidth=0.7)
    
    # Add percentage labels inside each segment
    for i, protocol in enumerate(protocols):
        total = routing_means[i] + data_means[i] + idle_means[i]
        if total > 0:
            # Routing % (bottom segment)
            r_pct = (routing_means[i] / total) * 100
            if routing_means[i] > total * 0.05:  # Only show if segment is large enough
                ax.text(i, routing_means[i]/2, f'{r_pct:.1f}%',
                       ha='center', va='center', fontweight='bold', 
                       color='white', fontsize=10)
            
            # Data % (middle segment)
            d_pct = (data_means[i] / total) * 100
            y_data = routing_means[i] + data_means[i]/2
            if data_means[i] > total * 0.05:
                ax.text(i, y_data, f'{d_pct:.1f}%',
                       ha='center', va='center', fontweight='bold', 
                       color='white', fontsize=10)
            
            # Idle % (top segment)
            i_pct = (idle_means[i] / total) * 100
            y_idle = routing_means[i] + data_means[i] + idle_means[i]/2
            if idle_means[i] > total * 0.05:
                ax.text(i, y_idle, f'{i_pct:.1f}%',
                       ha='center', va='center', fontweight='bold', 
                       color='white', fontsize=10)
            
            # Total energy label on top
            ax.text(i, total + (total * 0.02), f'{total:.1f}J',
                   ha='center', va='bottom', fontweight='bold', fontsize=11,
                   bbox=dict(facecolor='white', alpha=0.7, pad=1, edgecolor='none'))
    
    # Styling
    ax.set_ylabel('Energy Consumption (Joules)', fontweight='bold', 
                 fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    ax.set_xlabel('Routing Protocol', fontweight='bold', 
                 fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    ax.set_title('Energy Breakdown by Component (Phase 0 Model)',
                fontweight='bold', fontsize=CHART_CONFIG['title_fontsize'], pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels(protocols, fontsize=CHART_CONFIG['tick_fontsize'], fontweight='bold')
    ax.legend(loc='upper right', fontsize=CHART_CONFIG['legend_fontsize'], 
             frameon=True, framealpha=0.9)
    ax.grid(axis='y', alpha=CHART_CONFIG['grid_alpha'], 
           linestyle=CHART_CONFIG['grid_linestyle'])
    ax.set_axisbelow(True)
    
    plt.tight_layout()
    
    return fig_to_base64(fig)


def generate_routing_overhead_comparison(df: pd.DataFrame) -> str:
    """
    Generate grouped bar chart comparing routing vs data packets
    Shows protocol efficiency in terms of packet overhead
    
    Args:
        df: DataFrame with routingPackets and dataPackets columns
        
    Returns:
        Base64 encoded PNG image
    """
    
    if 'protocol' not in df.columns:
        return None
    
    if 'routingPackets' not in df.columns or 'dataPackets' not in df.columns:
        return None
    
    protocols = sorted(df['protocol'].unique())
    fig, ax = plt.subplots(figsize=CHART_CONFIG['figsize'])
    
    routing_means = []
    data_means = []
    routing_stds = []
    data_stds = []
    
    for protocol in protocols:
        proto_df = df[df['protocol'] == protocol]
        routing_means.append(proto_df['routingPackets'].mean())
        data_means.append(proto_df['dataPackets'].mean())
        routing_stds.append(proto_df['routingPackets'].std() if len(proto_df) > 1 else 0)
        data_stds.append(proto_df['dataPackets'].std() if len(proto_df) > 1 else 0)
    
    x = np.arange(len(protocols))
    width = 0.35
    
    # Grouped bars
    bars1 = ax.bar(x - width/2, routing_means, width, label='Routing Packets',
                  yerr=routing_stds, capsize=3,
                  color='#C62828', alpha=0.9, edgecolor='black', linewidth=0.7)
    bars2 = ax.bar(x + width/2, data_means, width, label='Data Packets',
                  yerr=data_stds, capsize=3,
                  color='#1565C0', alpha=0.9, edgecolor='black', linewidth=0.7)
    
    # Add value labels
    max_height = 0
    for bars, means, stds in [(bars1, routing_means, routing_stds), 
                               (bars2, data_means, data_stds)]:
        for bar, mean, std in zip(bars, means, stds):
            height = bar.get_height()
            if height + std > max_height:
                max_height = height + std
            if height > 0:
                label_text = f"{mean:.0f}\n±{std:.0f}"
                y_pos = height + std + (height * 0.02)
                ax.text(bar.get_x() + bar.get_width()/2., y_pos,
                       label_text, ha='center', va='bottom', fontsize=8,
                       bbox=dict(facecolor='white', alpha=0.7, pad=0.5, edgecolor='none'))
    
    # Styling
    ax.set_ylabel('Packet Count', fontweight='bold', 
                 fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    ax.set_xlabel('Routing Protocol', fontweight='bold', 
                 fontsize=CHART_CONFIG['label_fontsize'], labelpad=10)
    ax.set_title('Routing vs Data Packet Comparison',
                fontweight='bold', fontsize=CHART_CONFIG['title_fontsize'], pad=15)
    ax.set_xticks(x)
    ax.set_xticklabels(protocols, fontsize=CHART_CONFIG['tick_fontsize'], fontweight='bold')
    ax.legend(fontsize=CHART_CONFIG['legend_fontsize'], frameon=True, framealpha=0.9)
    ax.grid(axis='y', alpha=CHART_CONFIG['grid_alpha'], 
           linestyle=CHART_CONFIG['grid_linestyle'])
    ax.set_axisbelow(True)
    ax.set_ylim(bottom=0, top=max_height * 1.25 if max_height > 0 else 1)
    
    plt.tight_layout()
    
    return fig_to_base64(fig)


def fig_to_base64(fig) -> str:
    """
    Convert matplotlib figure to base64 encoded PNG
    
    Args:
        fig: Matplotlib figure object
        
    Returns:
        Base64 encoded PNG as data URI string
    """
    buffer = io.BytesIO()
    fig.savefig(buffer, format='png', dpi=CHART_CONFIG['dpi'], 
               bbox_inches='tight', facecolor='white')
    buffer.seek(0)
    image_base64 = base64.b64encode(buffer.read()).decode('utf-8')
    plt.close(fig)
    return f'data:image/png;base64,{image_base64}'


def calculate_statistics(df: pd.DataFrame) -> Dict[str, Any]:
    """
    Calculate comprehensive statistics for each protocol
    
    Args:
        df: DataFrame with simulation results
        
    Returns:
        Dictionary with statistics per protocol
    """
    stats = {}
    
    if 'protocol' not in df.columns:
        return stats
    
    for protocol in df['protocol'].unique():
        proto_df = df[df['protocol'] == protocol]
        
        protocol_stats = {
            'runs': len(proto_df),
        }
        
        # PDR statistics
        if 'pdr_percent' in proto_df.columns:
            protocol_stats['pdr_mean'] = float(proto_df['pdr_percent'].mean())
            protocol_stats['pdr_std'] = float(proto_df['pdr_percent'].std()) if len(proto_df) > 1 else 0
            protocol_stats['pdr_min'] = float(proto_df['pdr_percent'].min())
            protocol_stats['pdr_max'] = float(proto_df['pdr_percent'].max())
        
        # Energy statistics
        if 'consumedEnergy' in proto_df.columns:
            protocol_stats['energy_mean'] = float(proto_df['consumedEnergy'].mean())
            protocol_stats['energy_std'] = float(proto_df['consumedEnergy'].std()) if len(proto_df) > 1 else 0
            protocol_stats['energy_min'] = float(proto_df['consumedEnergy'].min())
            protocol_stats['energy_max'] = float(proto_df['consumedEnergy'].max())
        
        # Energy breakdown
        if all(col in proto_df.columns for col in ['routingEnergyJ', 'dataEnergyJ', 'idleEnergyJ']):
            total_energy = proto_df['routingEnergyJ'].mean() + proto_df['dataEnergyJ'].mean() + proto_df['idleEnergyJ'].mean()
            if total_energy > 0:
                protocol_stats['routing_energy_pct'] = float((proto_df['routingEnergyJ'].mean() / total_energy) * 100)
                protocol_stats['data_energy_pct'] = float((proto_df['dataEnergyJ'].mean() / total_energy) * 100)
                protocol_stats['idle_energy_pct'] = float((proto_df['idleEnergyJ'].mean() / total_energy) * 100)
        
        # Routing overhead
        if 'routingPackets' in proto_df.columns:
            protocol_stats['routing_packets_mean'] = float(proto_df['routingPackets'].mean())
            protocol_stats['routing_packets_std'] = float(proto_df['routingPackets'].std()) if len(proto_df) > 1 else 0
        
        if 'dataPackets' in proto_df.columns:
            protocol_stats['data_packets_mean'] = float(proto_df['dataPackets'].mean())
        
        # Energy efficiency
        if 'energy_efficiency' in proto_df.columns:
            protocol_stats['efficiency_mean'] = float(proto_df['energy_efficiency'].mean())
            protocol_stats['efficiency_std'] = float(proto_df['energy_efficiency'].std()) if len(proto_df) > 1 else 0
        
        stats[protocol] = protocol_stats
    
    return stats


def generate_all_charts(csv_path: str, chart_types: Optional[List[str]] = None) -> Dict[str, Any]:
    """
    Generate all visualization charts for GAODV dashboard
    Adapted from plot_comprehensive.py main() function
    
    Args:
        csv_path: Path to CSV results file
        chart_types: List of chart types to generate (None = all)
        
    Returns:
        Dictionary with chart images and statistics
    """
    
    try:
        # Load data
        df = pd.read_csv(csv_path)
        
        # Calculate additional metrics
        df = calculate_metrics(df)
        
        result = {}
        
        # Always include statistics
        result['statistics'] = calculate_statistics(df)
        
        # Default chart types
        if chart_types is None:
            chart_types = ['all']
        
        # Energy breakdown (Phase 0 specific)
        if 'all' in chart_types or 'energy_breakdown' in chart_types:
            breakdown = generate_energy_breakdown_stacked(df)
            if breakdown:
                result['energy_breakdown'] = breakdown
        
        # Routing overhead comparison
        if 'all' in chart_types or 'routing_overhead' in chart_types:
            overhead = generate_routing_overhead_comparison(df)
            if overhead:
                result['routing_overhead'] = overhead
        
        # Matrix of comparison charts (from reference)
        if 'all' in chart_types or 'matrix' in chart_types:
            # Independent variables (X-axis)
            x_vars = ['nodes', 'txPower', 'simtime']
            
            # Dependent variables (Y-axis)
            y_vars = ['pdr_percent', 'consumedEnergy', 'energy_efficiency', 
                     'routing_overhead_pct']
            
            # Generate charts for varying X variables
            for x_var in x_vars:
                if x_var in df.columns and df[x_var].nunique() > 1:
                    for y_var in y_vars:
                        if y_var in df.columns:
                            chart_key = f"{y_var}_by_{x_var}"
                            chart = generate_grouped_bar_plot(df, x_var, y_var)
                            if chart:
                                result[chart_key] = chart
        
        # Legacy charts for backward compatibility
        if 'pdr' in chart_types or 'all' in chart_types:
            if 'nodes' in df.columns and df['nodes'].nunique() > 1:
                pdr_chart = generate_grouped_bar_plot(df, 'nodes', 'pdr_percent', 
                                                     'PDR Comparison')
                if pdr_chart:
                    result['pdr_chart'] = pdr_chart
        
        if 'energy' in chart_types or 'all' in chart_types:
            if 'nodes' in df.columns and df['nodes'].nunique() > 1:
                energy_chart = generate_grouped_bar_plot(df, 'nodes', 'consumedEnergy',
                                                        'Energy Consumption')
                if energy_chart:
                    result['energy_chart'] = energy_chart
        
        if 'efficiency' in chart_types or 'all' in chart_types:
            if 'nodes' in df.columns and df['nodes'].nunique() > 1:
                eff_chart = generate_grouped_bar_plot(df, 'nodes', 'energy_efficiency',
                                                     'Energy Efficiency')
                if eff_chart:
                    result['efficiency_chart'] = eff_chart
        
        return result
        
    except Exception as e:
        return {'error': str(e)}


def load_csv_data(csv_path: str) -> Dict[str, Any]:
    """
    Load and return CSV data as JSON
    
    Args:
        csv_path: Path to CSV file
        
    Returns:
        Dictionary with columns and data
    """
    try:
        df = pd.read_csv(csv_path)
        return {
            'columns': list(df.columns),
            'data': df.to_dict('records'),
            'row_count': len(df)
        }
    except Exception as e:
        return {'error': str(e)}
