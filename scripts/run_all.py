#!/usr/bin/env python3
"""
MANET Protocol Comparison Automation Tool
==========================================
Automates running NS-3 MANET simulations with parameter sweeping.
Based on the methodology from NS3-Research project.

Supports GAODV version selection:
    --gaodv-version v0  → Uses gaodv-v0/ (original)
    --gaodv-version v1  → Uses gaodv-v1/ (geometric/energy-aware)

Usage:
    python3 run_all.py --protocols GAODV AODV --runs 10
    python3 run_all.py --protocols GAODV --gaodv-version v1 --runs 5
    python3 run_all.py --protocols GAODV-V0 GAODV-V1 AODV --runs 10
"""

import subprocess
import os
import sys
import argparse
import time
import re
import shutil
from pathlib import Path
from datetime import datetime

# ========== NS-3 AUTO-DETECTION ==========
def find_ns3_installation():
    """Find ns-3 installation by scanning common locations."""
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    
    ns3_version_pattern = re.compile(r'ns-3\.\d+$')
    ns3_allinone_pattern = re.compile(r'ns-allinone-3\.\d+$')
    
    found_installations = []
    
    print("Searching for ns-3 installation...")
    
    # Search in home directory
    home_dir = Path.home()
    try:
        for item in home_dir.iterdir():
            if not item.is_dir() or item.name.startswith('.'):
                continue
            
            try:
                # Check for ns-allinone
                if ns3_allinone_pattern.match(item.name):
                    for subitem in item.iterdir():
                        if subitem.is_dir() and ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists() or (subitem / "waf").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
                                break
                
                # Check for direct ns-3 directory
                elif ns3_version_pattern.match(item.name):
                    if (item / "ns3").exists() or (item / "waf").exists():
                        found_installations.append(item)
                        print(f"  Found: {item}")
                
                # NEW: Check one level deeper for nested installations
                else:
                    for subitem in item.iterdir():
                        if not subitem.is_dir():
                            continue
                        
                        # Check for ns-allinone in subdirectory
                        if ns3_allinone_pattern.match(subitem.name):
                            for subsubitem in subitem.iterdir():
                                if subsubitem.is_dir() and ns3_version_pattern.match(subsubitem.name):
                                    if (subsubitem / "ns3").exists() or (subsubitem / "waf").exists():
                                        found_installations.append(subsubitem)
                                        print(f"  Found: {subsubitem}")
                                        break
                        
                        # Check for direct ns-3 in subdirectory
                        elif ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists() or (subitem / "waf").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
            
            except (PermissionError, OSError):
                continue
    
    except (PermissionError, OSError):
        print("  Warning: Limited access to home directory")
    
    # Search in system paths
    system_paths = [Path("/usr/local/ns3"), Path("/usr/local"), Path("/opt")]
    
    for base in system_paths:
        if not base.exists():
            continue
        
        try:
            for item in base.iterdir():
                if not item.is_dir():
                    continue
                
                if ns3_allinone_pattern.match(item.name):
                    for subitem in item.iterdir():
                        if subitem.is_dir() and ns3_version_pattern.match(subitem.name):
                            if (subitem / "ns3").exists() or (subitem / "waf").exists():
                                found_installations.append(subitem)
                                print(f"  Found: {subitem}")
                                break
                
                elif ns3_version_pattern.match(item.name):
                    if (item / "ns3").exists() or (item / "waf").exists():
                        found_installations.append(item)
                        print(f"  Found: {item}")
        
        except (PermissionError, OSError):
            continue
    
    if found_installations:
        unique_installations = list(set(found_installations))
        unique_installations.sort(key=lambda p: p.name, reverse=True)
        detected = unique_installations[0]
        print(f"\n✓ Auto-detected ns-3 version: {detected.name}")
        print(f"  Location: {detected}")
        return detected
    
    print("\nERROR: Could not auto-detect ns-3 installation.")
    print(f"Searched in: {home_dir} and system directories")
    print("Please specify --ns3-path manually")
    return None


# ========== CONFIGURATION ==========
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent

# Auto-discover source file (like NS3-Research approach)
SRC_DIR = PROJECT_DIR / "src"
# Find the .cc file in src directory
cc_files = list(SRC_DIR.glob("*.cc"))
if not cc_files:
    print("ERROR: No .cc file found in src/ directory")
    sys.exit(1)

SOURCE_FILE = cc_files[0]  # Use first .cc file found
# Sanitize filename: remove trailing space and replace internal spaces with underscores
# CMake doesn't like spaces in target names
PROGRAM_NAME = SOURCE_FILE.stem.strip().replace(' ', '_')

print(f"Project Directory: {PROJECT_DIR}")
print(f"Discovered source file: {SOURCE_FILE.name}")
print(f"Sanitized program name: {PROGRAM_NAME}")

# Try to find NS-3
NS3_PATH = None


# ========== GAODV HELPERS ==========
def get_gaodv_source_dir():
    """Get source directory for GAODV (currently V1 implementation)."""
    return PROJECT_DIR / "gaodv-v1"


def uses_gaodv(protocols):
    """Check if any protocol uses GAODV."""
    return 'GAODV' in [p.upper() for p in protocols]


# ========== SETUP FUNCTIONS ==========
def setup_simulation(ns3_path, source_file, copy_gaodv=False):
    """Copy simulation file to scratch folder and build.
    
    Args:
        ns3_path: Path to NS-3 installation
        source_file: Main simulation source file
        copy_gaodv: Whether to copy GAODV module to NS-3
    """
    scratch_path = ns3_path / "scratch"
    # Use sanitized filename (CMake doesn't like spaces or trailing spaces)
    dest_file = scratch_path / f"{PROGRAM_NAME}.cc"
    
    if not source_file.exists():
        print(f"ERROR: Source file not found: {source_file}")
        sys.exit(1)
    
    print(f"\nCopying {source_file.name} to {dest_file.name}...")
    shutil.copy2(source_file, dest_file)
    
    # Copy GAODV module to NS-3 src directory if needed
    if copy_gaodv:
        ns3_src = ns3_path / "src"
        gaodv_src = get_gaodv_source_dir()
        if gaodv_src.exists():
            ns3_gaodv_dst = ns3_src / "gaodv-v1"
            print(f"Copying GAODV module to NS-3 src/...")
            if ns3_gaodv_dst.exists():
                shutil.rmtree(ns3_gaodv_dst)
            shutil.copytree(gaodv_src, ns3_gaodv_dst)
            print(f"  → {ns3_gaodv_dst}")
        else:
            print(f"WARNING: GAODV source not found at {gaodv_src}")
    
    print("Building ns-3...")
    
    # Determine build command (ns3 or ./waf)
    if (ns3_path / "ns3").exists():
        build_cmd = ["./ns3", "build"]
    else:
        build_cmd = ["./waf", "build"]
    
    process = subprocess.Popen(
        build_cmd,
        cwd=ns3_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    for line in process.stdout:
        print(line, end='', flush=True)
    
    process.wait()
    
    if process.returncode != 0:
        print(f"ERROR: Build failed!")
        sys.exit(1)
    
    print("✓ Build successful!\n")


# ========== EXPERIMENT EXECUTION ==========
def run_single_experiment(ns3_path, protocol, nodes, txpower, run_number, sim_args):
    """Run a single simulation experiment.
    
    Args:
        ns3_path: Path to NS-3 installation
        protocol: Protocol name
        nodes: Number of nodes
        txpower: TX power
        run_number: Run number
        sim_args: Dictionary of additional simulation arguments
    """
    print(f"  Running: {protocol} | Nodes: {nodes} | TxPower: {txpower} | Run: {run_number}")
    
    # Build NS-3 command arguments
    ns3_args = f"--protocol={protocol} --nWifis={nodes} --txpower={txpower}"
    
    # Add run tracking for session-based CSV output
    if sim_args.get('session_id'):
        ns3_args += f" --sessionId={sim_args['session_id']}"
    ns3_args += f" --runNumber={run_number}"
    
    # Add optional simulation parameters from EDA-AODV paper
    if sim_args.get('simtime'):
        ns3_args += f" --totalTime={sim_args['simtime']}"
    if sim_args.get('sinks'):
        ns3_args += f" --nSinks={sim_args['sinks']}"
    if sim_args.get('initial_energy'):
        ns3_args += f" --initialEnergy={sim_args['initial_energy']}"
    if sim_args.get('packet_size'):
        ns3_args += f" --packetSize={sim_args['packet_size']}"
    if sim_args.get('packet_rate'):
        ns3_args += f" --packetRate={sim_args['packet_rate']}"
    if sim_args.get('txrange'):
        ns3_args += f" --txRange={sim_args['txrange']}"
    
    # Determine run command - use string for shell=True
    if (ns3_path / "ns3").exists():
        run_cmd = f'./ns3 run "{PROGRAM_NAME} {ns3_args}"'
    else:
        run_cmd = f'./waf --run "{PROGRAM_NAME} {ns3_args}"'
    
    try:
        result = subprocess.run(
            run_cmd,
            cwd=ns3_path,
            capture_output=True,
            text=True,
            timeout=900,  # 15 minute timeout (simulations can run 500+ seconds)
            shell=True
        )
        
        # Parse metrics from output (check both stdout and stderr as NS-3 logs to stderr)
        # NOTE: NS-3 often returns SIGSEGV (245/11) during Simulator::Destroy() cleanup
        # even when simulation completed successfully. Check for actual results instead.
        output = result.stdout + result.stderr
        pdr_match = re.search(r'PDR:\s+([\d.]+)%', output)
        sent_match = re.search(r'Sender Sent:\s+(\d+)', output)
        recv_match = re.search(r'Sink Received.*:\s+(\d+)', output)
        energy_match = re.search(r'Total Consumed Energy:\s+([\d.]+)', output)
        
        # Simulation succeeded if we got PDR output (even if returncode != 0 due to cleanup crash)
        if pdr_match:
            # Build output string with all available metrics
            metrics = []
            metrics.append(f"PDR: {pdr_match.group(1)}%")
            
            if sent_match and recv_match:
                sent = int(sent_match.group(1))
                recv = int(recv_match.group(1))
                metrics.append(f"Sent: {sent}")
                metrics.append(f"Recv: {recv}")
                # Calculate throughput (packets per 500 seconds with 64 byte packets)
                if sent > 0:
                    throughput_kbps = (recv * 64 * 8) / 500 / 1000
                    metrics.append(f"Throughput: {throughput_kbps:.2f} kbps")
            
            if energy_match:
                metrics.append(f"Energy: {float(energy_match.group(1)):.1f}J")
            
            print(f"    ✓ {' | '.join(metrics)}")
            return True
        else:
            # True failure - no results produced
            print(f"    ✗ Failed")
            error_output = result.stderr[:200] if result.stderr else 'No error output'
            print(f"       Error: {error_output}")
            return False
    
    except subprocess.TimeoutExpired:
        print(f"    ✗ Timeout")
        return False
    except Exception as e:
        print(f"    ✗ Exception: {e}")
        return False


def run_experiments(args):
    """Run all experiments with parameter sweeping."""
    print(f"\n{'='*70}")
    print(f"RUNNING EXPERIMENTS")
    print(f"{'='*70}\n")
    
    # Check if GAODV is used
    needs_gaodv = uses_gaodv(args.protocols)
    
    # Set up simulation if not skipping build
    if not args.skip_build:
        setup_simulation(args.ns3_path, args.source_file, copy_gaodv=needs_gaodv)
    
    # Generate experiments (protocols used directly, no version mapping)
    experiments = []
    for protocol in args.protocols:
        for node_count in args.nodes:
            for txpower in args.txpower:
                for run in range(1, args.runs + 1):
                    experiments.append((protocol.upper(), node_count, txpower, run))
    
    print(f"Total experiments to run: {len(experiments)}\n")
    
    # Run experiments
    successful = 0
    failed = 0
    
    # Generate session ID for this batch of experiments
    session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    print(f"Session ID: {session_id}\n")
    
    # Build simulation arguments dictionary
    sim_args = {
        'simtime': args.simtime,
        'sinks': args.sinks,
        'initial_energy': args.initial_energy,
        'packet_size': args.packet_size,
        'packet_rate': args.packet_rate,
        'txrange': args.txrange[0] if args.txrange else 100.0,  # Use first txrange value
        'session_id': session_id,  # Add session ID for CSV grouping
    }
    
    for i, (protocol, nodes, txpower, run) in enumerate(experiments, 1):
        print(f"[{i}/{len(experiments)}]", end=" ")
        if run_single_experiment(args.ns3_path, protocol, nodes, txpower, run, sim_args):
            successful += 1
        else:
            failed += 1
    
    print(f"\n{'='*70}")
    print(f"EXPERIMENT SUMMARY")
    print(f"{'='*70}")
    print(f"Total: {len(experiments)} | Successful: {successful} | Failed: {failed}")
    print(f"{'='*70}\n")


# ========== RESULT COLLECTION ==========
def collect_results(args):
    """Move individual result CSV files to results folder (no aggregation)."""
    print(f"\n{'='*70}")
    print(f"COLLECTING RESULTS")
    print(f"{'='*70}\n")
    
    results_dir = PROJECT_DIR / "results"
    results_dir.mkdir(exist_ok=True)
    
    # Look for result CSV files in NS-3 directory (where simulation creates them)
    result_files = list(args.ns3_path.glob("results_*.csv"))
    
    if not result_files:
        print("No result files found to collect.")
        print(f"Searched in: {args.ns3_path}")
        return
    
    # Move each result file to results folder (preserving individual files)
    moved_count = 0
    for result_file in result_files:
        dest_file = results_dir / result_file.name
        
        # If file already exists, add counter to avoid overwrite
        if dest_file.exists():
            base = result_file.stem
            ext = result_file.suffix
            counter = 1
            while dest_file.exists():
                dest_file = results_dir / f"{base}_{counter}{ext}"
                counter += 1
        
        shutil.move(str(result_file), str(dest_file))
        print(f"  ✓ {result_file.name} → results/{dest_file.name}")
        moved_count += 1
    
    print(f"\n✓ Collected {moved_count} result file(s) to: {results_dir}")
    print(f"{'='*70}\n")


# ========== MAIN ==========
def main():
    parser = argparse.ArgumentParser(
        description="Automate MANET protocol comparison experiments",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run GAODV vs AODV comparison
  python3 run_all.py --protocols GAODV AODV --runs 10
  
  # Parameter sweep
  python3 run_all.py --protocols GAODV --nodes 25 50 100 --txpower 5.0 7.5 10.0
  
  # Quick test (skip build)
  python3 run_all.py --protocols GAODV --runs 3 --skip-build
        """
    )
    
    parser.add_argument('--protocols', nargs='+', default=['GAODV'],
                        choices=['GAODV', 'AODV', 'OLSR', 'DSDV', 'DSR'],
                        help='Protocols to test')
    parser.add_argument('--nodes', nargs='+', type=int, default=[50],
                        help='Number of nodes to test (default: 50 per EDA-AODV paper)')
    parser.add_argument('--txpower', nargs='+', type=float, default=[7.5],
                        help='Transmission power values (dBm)')
    parser.add_argument('--txrange', nargs='+', type=float, default=[100.0],
                        help='Transmission range in meters (default: 100)')
    parser.add_argument('--runs', type=int, default=5,
                        help='Number of runs per configuration')
    
    # EDA-AODV Paper Simulation Parameters (Table 1)
    parser.add_argument('--simtime', type=int, default=500,
                        help='Simulation time in seconds (default: 500)')
    parser.add_argument('--sinks', type=int, default=10,
                        help='Number of sink nodes (default: 10)')
    parser.add_argument('--initial-energy', type=float, default=20.0, dest='initial_energy',
                        help='Initial energy per node in Joules (default: 20)')
    parser.add_argument('--packet-size', type=int, default=512, dest='packet_size',
                        help='Packet size in bytes (default: 512)')
    parser.add_argument('--packet-rate', type=int, default=4, dest='packet_rate',
                        help='Packets per second (default: 4)')
    parser.add_argument('--ethf', type=float, default=0.2,
                        help='Energy Threshold Factor for GAODV (default: 0.2, range 0-1)')
    
    # System options
    parser.add_argument('--ns3-path', type=Path, default=None,
                        help='Path to NS-3 installation (auto-detected if not specified)')
    parser.add_argument('--source-file', type=Path, default=None,
                        help='Path to simulation source file')
    parser.add_argument('--skip-build', action='store_true',
                        help='Skip building NS-3 (assumes already built)')
    parser.add_argument('--no-collect', action='store_true',
                        help='Skip collecting result files to results folder')
    
    args = parser.parse_args()
    
    # Auto-detect NS-3 if not provided
    if args.ns3_path is None:
        args.ns3_path = find_ns3_installation()
        if args.ns3_path is None:
            sys.exit(1)
    else:
        args.ns3_path = args.ns3_path.resolve()
    
    # Set source file if not provided
    if args.source_file is None:
        args.source_file = SOURCE_FILE
    
    print(f"\nConfiguration:")
    print(f"  NS-3 Path: {args.ns3_path}")
    print(f"  Source File: {args.source_file}")
    print(f"  Protocols: {args.protocols}")
    print(f"  Nodes: {args.nodes}")
    print(f"  TX Range: {args.txrange[0] if args.txrange else 100.0}m")
    print(f"  TX Power: {args.txpower}")
    print(f"  Runs: {args.runs}")
    print(f"  Simulation Time: {args.simtime}s")
    print(f"  Sinks: {args.sinks}")
    print(f"  Initial Energy: {args.initial_energy}J")
    print(f"  Packet Size: {args.packet_size}B")
    print(f"  Packet Rate: {args.packet_rate} pkt/s")
    print()
    
    # Run experiments
    start_time = time.time()
    run_experiments(args)
    
    # Collect results (move individual files to results folder)
    if not args.no_collect:
        collect_results(args)
    
    elapsed_time = time.time() - start_time
    print(f"Total execution time: {elapsed_time:.2f} seconds")
    print("\nDone!")


if __name__ == "__main__":
    main()
