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
def run_single_experiment(ns3_path, protocol, nodes, txpower, run_number):
    """Run a single simulation experiment."""
    print(f"  Running: {protocol} | Nodes: {nodes} | TxPower: {txpower} | Run: {run_number}")
    
    # Determine run command - use string for shell=True
    if (ns3_path / "ns3").exists():
        run_cmd = f'./ns3 run "{PROGRAM_NAME} --protocol={protocol}"'
    else:
        run_cmd = f'./waf --run "{PROGRAM_NAME} --protocol={protocol}"'
    
    try:
        result = subprocess.run(
            run_cmd,
            cwd=ns3_path,
            capture_output=True,
            text=True,
            timeout=300,  # 5 minute timeout
            shell=True
        )
        
        if result.returncode == 0:
            # Parse PDR from output (check both stdout and stderr as NS-3 logs to stderr)
            output = result.stdout + result.stderr
            pdr_match = re.search(r'PDR:\s+([\d.]+)%', output)
            sent_match = re.search(r'Sender Sent:\s+(\d+)', output)
            recv_match = re.search(r'Sink Received.*:\s+(\d+)', output)
            
            pdr_str = f"PDR: {pdr_match.group(1)}%" if pdr_match else "PDR: N/A"
            if sent_match and recv_match:
                pdr_str += f" (Sent: {sent_match.group(1)}, Recv: {recv_match.group(1)})"
            
            print(f"    ✓ {pdr_str}")
            return True
        else:
            print(f"    ✗ Failed")
            print(f"       Error: {result.stderr[:200] if result.stderr else 'No error output'}")
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
    
    for i, (protocol, nodes, txpower, run) in enumerate(experiments, 1):
        print(f"[{i}/{len(experiments)}]", end=" ")
        if run_single_experiment(args.ns3_path, protocol, nodes, txpower, run):
            successful += 1
        else:
            failed += 1
    
    print(f"\n{'='*70}")
    print(f"EXPERIMENT SUMMARY")
    print(f"{'='*70}")
    print(f"Total: {len(experiments)} | Successful: {successful} | Failed: {failed}")
    print(f"{'='*70}\n")


# ========== RESULT AGGREGATION ==========
def aggregate_results(args):
    """Aggregate protocol CSV files into a single master CSV."""
    print(f"\n{'='*70}")
    print(f"AGGREGATING RESULTS")
    print(f"{'='*70}\n")
    
    results_dir = PROJECT_DIR / "results"
    results_dir.mkdir(exist_ok=True)
    
    # Look for result CSV files in NS-3 directory (where simulation creates them)
    result_files = list(args.ns3_path.glob("results_*.csv"))
    
    if not result_files:
        print("No result files found to aggregate.")
        print(f"Searched in: {args.ns3_path}")
        return
    
    # Aggregate all results
    aggregated_file = results_dir / "aggregated_results.csv"
    
    with open(aggregated_file, 'w') as outfile:
        header_written = False
        
        for result_file in result_files:
            print(f"  Adding: {result_file.name}")
            with open(result_file, 'r') as infile:
                lines = infile.readlines()
                if not lines:
                    continue
                
                # Write header from first file only
                if not header_written:
                    outfile.write(lines[0])
                    header_written = True
                
                # Write data lines (skip header)
                for line in lines[1:]:
                    outfile.write(line)
    
    print(f"\n✓ Aggregated results saved to: {aggregated_file}")
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
                        help='Number of nodes to test')
    parser.add_argument('--txpower', nargs='+', type=float, default=[7.5],
                        help='Transmission power values (dBm)')
    parser.add_argument('--runs', type=int, default=5,
                        help='Number of runs per configuration')
    parser.add_argument('--ns3-path', type=Path, default=None,
                        help='Path to NS-3 installation (auto-detected if not specified)')
    parser.add_argument('--source-file', type=Path, default=None,
                        help='Path to simulation source file')
    parser.add_argument('--skip-build', action='store_true',
                        help='Skip building NS-3 (assumes already built)')
    parser.add_argument('--no-aggregate', action='store_true',
                        help='Skip result aggregation')
    
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
    print(f"  TX Power: {args.txpower}")
    print(f"  Runs: {args.runs}")
    print()
    
    # Run experiments
    start_time = time.time()
    run_experiments(args)
    
    # Aggregate results
    if not args.no_aggregate:
        aggregate_results(args)
    
    elapsed_time = time.time() - start_time
    print(f"Total execution time: {elapsed_time:.2f} seconds")
    print("\nDone!")


if __name__ == "__main__":
    main()
