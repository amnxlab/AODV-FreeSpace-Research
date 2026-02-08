"""
Result Visualization Module for GAODV Dashboard
Generates charts and graphs from CSV result files
"""

import pandas as pd
import io
import base64
from pathlib import Path
from typing import Dict, List, Optional, Any
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend for server-side rendering
import matplotlib.pyplot as plt
import numpy as np


def generate_charts(csv_path: str, chart_types: Optional[List[str]] = None) -> Dict[str, Any]:
    """
    Generate charts from CSV result file
    
    Args:
        csv_path: Path to CSV result file
        chart_types: List of chart types to generate. Options:
                    ['pdr', 'energy', 'throughput', 'efficiency']
                    If None, generates all charts
                    
    Returns:
        Dictionary with chart data:
        {
            'pdr_chart': base64 encoded PNG,
            'energy_chart': base64 encoded PNG,
            'efficiency_chart': base64 encoded PNG,
            'statistics': dict of calculated statistics
        }
    """
    try:
        # Read CSV file
        df = pd.read_csv(csv_path)
        
        if chart_types is None:
            chart_types = ['pdr', 'energy', 'efficiency', 'overview']
            
        result = {}
        
        # Calculate statistics
        result['statistics'] = calculate_statistics(df)
        
        # Generate requested charts
        if 'pdr' in chart_types:
            result['pdr_chart'] = generate_pdr_chart(df)
            
        if 'energy' in chart_types:
            result['energy_chart'] = generate_energy_chart(df)
            
        if 'efficiency' in chart_types:
            result['efficiency_chart'] = generate_efficiency_chart(df)
            
        if 'overview' in chart_types:
            result['overview_chart'] = generate_overview_chart(df)
            
        return result
        
    except Exception as e:
        return {'error': str(e)}


def calculate_statistics(df: pd.DataFrame) -> Dict[str, Any]:
    """Calculate summary statistics from dataframe"""
    stats = {}
    
    # Group by protocol if multiple protocols present
    if 'protocol' in df.columns:
        protocols = df['protocol'].unique()
        
        for protocol in protocols:
            protocol_df = df[df['protocol'] == protocol]
            
            stats[protocol] = {
                'pdr_mean': float(protocol_df['pdr'].mean() * 100),
                'pdr_std': float(protocol_df['pdr'].std() * 100),
                'pdr_min': float(protocol_df['pdr'].min() * 100),
                'pdr_max': float(protocol_df['pdr'].max() * 100),
                'energy_consumed_mean': float(protocol_df['consumedEnergy'].mean()),
                'energy_consumed_std': float(protocol_df['consumedEnergy'].std()),
                'energy_residual_mean': float(protocol_df['residualEnergy'].mean()),
                'efficiency': float(protocol_df['pdr'].mean() / (protocol_df['consumedEnergy'].mean() / 1000))
            }
    else:
        # Single protocol
        stats['overall'] = {
            'pdr_mean': float(df['pdr'].mean() * 100),
            'pdr_std': float(df['pdr'].std() * 100),
            'energy_consumed_mean': float(df['consumedEnergy'].mean()),
            'energy_consumed_std': float(df['consumedEnergy'].std()),
            'runs': len(df)
        }
        
    return stats


def generate_pdr_chart(df: pd.DataFrame) -> str:
    """Generate PDR comparison bar chart"""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    if 'protocol' in df.columns:
        # Multiple protocols - group by protocol
        protocols = df['protocol'].unique()
        pdr_means = []
        pdr_stds = []
        
        for protocol in protocols:
            protocol_df = df[df['protocol'] == protocol]
            pdr_means.append(protocol_df['pdr'].mean() * 100)
            pdr_stds.append(protocol_df['pdr'].std() * 100)
            
        # Color scheme - MANET themed
        colors = ['#0d9488', '#1a365d', '#059669', '#d97706', '#dc2626']
        
        bars = ax.bar(protocols, pdr_means, yerr=pdr_stds, capsize=5,
                     color=colors[:len(protocols)], alpha=0.8, edgecolor='black')
        
        ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
        ax.set_xlabel('Routing Protocol', fontsize=12, fontweight='bold')
        ax.set_title('PDR Comparison Across Protocols', fontsize=14, fontweight='bold')
        ax.set_ylim([0, 105])
        ax.grid(axis='y', alpha=0.3, linestyle='--')
        
        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{height:.2f}%',
                   ha='center', va='bottom', fontweight='bold')
                   
    else:
        # Single protocol - show runs
        runs = range(1, len(df) + 1)
        pdr_values = df['pdr'].values * 100
        
        ax.plot(runs, pdr_values, marker='o', linewidth=2, markersize=8,
               color='#0d9488', label='PDR')
        ax.axhline(y=pdr_values.mean(), color='#dc2626', linestyle='--',
                  label=f'Mean: {pdr_values.mean():.2f}%')
        
        ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
        ax.set_xlabel('Run Number', fontsize=12, fontweight='bold')
        ax.set_title('PDR Across Simulation Runs', fontsize=14, fontweight='bold')
        ax.set_ylim([0, 105])
        ax.grid(alpha=0.3)
        ax.legend()
        
    plt.tight_layout()
    return fig_to_base64(fig)


def generate_energy_chart(df: pd.DataFrame) -> str:
    """Generate energy consumption chart"""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    if 'protocol' in df.columns:
        protocols = df['protocol'].unique()
        consumed_means = []
        residual_means = []
        
        for protocol in protocols:
            protocol_df = df[df['protocol'] == protocol]
            consumed_means.append(protocol_df['consumedEnergy'].mean())
            residual_means.append(protocol_df['residualEnergy'].mean())
            
        x = np.arange(len(protocols))
        width = 0.35
        
        bars1 = ax.bar(x - width/2, consumed_means, width, label='Consumed',
                      color='#dc2626', alpha=0.8)
        bars2 = ax.bar(x + width/2, residual_means, width, label='Residual',
                      color='#059669', alpha=0.8)
        
        ax.set_ylabel('Energy (Joules)', fontsize=12, fontweight='bold')
        ax.set_xlabel('Routing Protocol', fontsize=12, fontweight='bold')
        ax.set_title('Energy Consumption Analysis', fontsize=14, fontweight='bold')
        ax.set_xticks(x)
        ax.set_xticklabels(protocols)
        ax.legend()
        ax.grid(axis='y', alpha=0.3, linestyle='--')
        
        # Add value labels
        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                ax.text(bar.get_x() + bar.get_width()/2., height,
                       f'{height:.1f}J',
                       ha='center', va='bottom', fontsize=9)
                       
    else:
        # Single protocol - stacked area chart
        runs = range(1, len(df) + 1)
        consumed = df['consumedEnergy'].values
        residual = df['residualEnergy'].values
        initial = consumed + residual
        
        ax.fill_between(runs, 0, consumed, label='Consumed',
                        color='#dc2626', alpha=0.6)
        ax.fill_between(runs, consumed, initial, label='Residual',
                        color='#059669', alpha=0.6)
        ax.plot(runs, initial, 'k--', linewidth=2, label='Initial', alpha=0.5)
        
        ax.set_ylabel('Energy (Joules)', fontsize=12, fontweight='bold')
        ax.set_xlabel('Run Number', fontsize=12, fontweight='bold')
        ax.set_title('Energy Distribution Across Runs', fontsize=14, fontweight='bold')
        ax.legend()
        ax.grid(alpha=0.3)
        
    plt.tight_layout()
    return fig_to_base64(fig)


def generate_efficiency_chart(df: pd.DataFrame) -> str:
    """Generate PDR vs Energy efficiency scatter plot"""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    if 'protocol' in df.columns:
        protocols = df['protocol'].unique()
        colors = ['#0d9488', '#1a365d', '#059669', '#d97706', '#dc2626']
        
        for i, protocol in enumerate(protocols):
            protocol_df = df[df['protocol'] == protocol]
            pdr = protocol_df['pdr'].values * 100
            energy = protocol_df['consumedEnergy'].values
            
            ax.scatter(energy, pdr, label=protocol, s=100, alpha=0.7,
                      color=colors[i % len(colors)], edgecolor='black', linewidth=1)
            
        ax.set_xlabel('Energy Consumed (Joules)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
        ax.set_title('Energy Efficiency Trade-off', fontsize=14, fontweight='bold')
        ax.legend()
        ax.grid(alpha=0.3)
        
        # Add ideal region annotation
        ax.axhline(y=95, color='green', linestyle=':', alpha=0.5, linewidth=2)
        ax.text(ax.get_xlim()[1] * 0.7, 96, 'High PDR Region',
               color='green', fontweight='bold', alpha=0.7)
               
    else:
        pdr = df['pdr'].values * 100
        energy = df['consumedEnergy'].values
        runs = range(1, len(df) + 1)
        
        scatter = ax.scatter(energy, pdr, c=runs, cmap='viridis', s=100,
                           alpha=0.7, edgecolor='black', linewidth=1)
        plt.colorbar(scatter, ax=ax, label='Run Number')
        
        ax.set_xlabel('Energy Consumed (Joules)', fontsize=12, fontweight='bold')
        ax.set_ylabel('Packet Delivery Ratio (%)', fontsize=12, fontweight='bold')
        ax.set_title('Run-by-Run Efficiency', fontsize=14, fontweight='bold')
        ax.grid(alpha=0.3)
        
    plt.tight_layout()
    return fig_to_base64(fig)


def generate_overview_chart(df: pd.DataFrame) -> str:
    """Generate overview dashboard with multiple metrics"""
    fig = plt.figure(figsize=(14, 8))
    
    if 'protocol' in df.columns:
        protocols = df['protocol'].unique()
        
        # Create 2x2 subplot grid
        ax1 = plt.subplot(2, 2, 1)
        ax2 = plt.subplot(2, 2, 2)
        ax3 = plt.subplot(2, 2, 3)
        ax4 = plt.subplot(2, 2, 4)
        
        colors = ['#0d9488', '#1a365d', '#059669', '#d97706', '#dc2626']
        
        # 1. PDR comparison
        pdr_means = [df[df['protocol'] == p]['pdr'].mean() * 100 for p in protocols]
        ax1.bar(protocols, pdr_means, color=colors[:len(protocols)], alpha=0.8)
        ax1.set_ylabel('PDR (%)', fontweight='bold')
        ax1.set_title('Packet Delivery Ratio', fontweight='bold')
        ax1.grid(axis='y', alpha=0.3)
        
        # 2. Energy consumption
        energy_means = [df[df['protocol'] == p]['consumedEnergy'].mean() for p in protocols]
        ax2.bar(protocols, energy_means, color=colors[:len(protocols)], alpha=0.8)
        ax2.set_ylabel('Energy (J)', fontweight='bold')
        ax2.set_title('Energy Consumption', fontweight='bold')
        ax2.grid(axis='y', alpha=0.3)
        
        # 3. Efficiency metric (PDR / Energy)
        efficiency = [pdr_means[i] / energy_means[i] for i in range(len(protocols))]
        ax3.bar(protocols, efficiency, color=colors[:len(protocols)], alpha=0.8)
        ax3.set_ylabel('PDR % / Joule', fontweight='bold')
        ax3.set_title('Energy Efficiency', fontweight='bold')
        ax3.grid(axis='y', alpha=0.3)
        
        # 4. Packets sent vs received
        packets_sent = [df[df['protocol'] == p]['packetsSent'].mean() for p in protocols]
        packets_received = [df[df['protocol'] == p]['packetsReceived'].mean() for p in protocols]
        
        x = np.arange(len(protocols))
        width = 0.35
        ax4.bar(x - width/2, packets_sent, width, label='Sent', color='#1a365d', alpha=0.8)
        ax4.bar(x + width/2, packets_received, width, label='Received', color='#0d9488', alpha=0.8)
        ax4.set_ylabel('Packets', fontweight='bold')
        ax4.set_title('Packet Statistics', fontweight='bold')
        ax4.set_xticks(x)
        ax4.set_xticklabels(protocols)
        ax4.legend()
        ax4.grid(axis='y', alpha=0.3)
        
    plt.suptitle('Simulation Results Overview', fontsize=16, fontweight='bold', y=0.995)
    try:
        plt.tight_layout()
    except ValueError:
        # Ignore tight_layout warnings for complex charts
        pass
    return fig_to_base64(fig)


def fig_to_base64(fig) -> str:
    """Convert matplotlib figure to base64 encoded PNG"""
    buffer = io.BytesIO()
    fig.savefig(buffer, format='png', dpi=100, bbox_inches='tight')
    buffer.seek(0)
    image_base64 = base64.b64encode(buffer.read()).decode('utf-8')
    plt.close(fig)
    return f'data:image/png;base64,{image_base64}'


def load_csv_data(csv_path: str) -> Dict[str, Any]:
    """Load and return CSV data as JSON"""
    try:
        df = pd.read_csv(csv_path)
        return {
            'columns': list(df.columns),
            'data': df.to_dict('records'),
            'row_count': len(df)
        }
    except Exception as e:
        return {'error': str(e)}
