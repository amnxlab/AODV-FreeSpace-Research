"""
Simulation Controller Module for GAODV Dashboard
Manages NS-3 simulation execution, output parsing, and progress tracking
"""

import os
import sys
import subprocess
import threading
import re
import time
from datetime import datetime
from typing import Dict, List, Optional, Callable, Any
from pathlib import Path


class SimulationRunner:
    """Manages simulation execution with real-time progress tracking"""
    
    def __init__(self, project_root: str):
        """
        Initialize simulation runner
        
        Args:
            project_root: Absolute path to AODV-FreeSpace-Research directory
        """
        self.project_root = Path(project_root)
        self.scripts_dir = self.project_root / 'scripts'
        self.results_dir = self.project_root / 'results'
        
        # Simulation state
        self.process: Optional[subprocess.Popen] = None
        self.thread: Optional[threading.Thread] = None
        self.is_running = False
        self.current_run = 0
        self.total_runs = 0
        self.current_protocol = ""
        self.session_id = ""
        self.start_time: Optional[float] = None
        
        # Output buffers
        self.stdout_buffer = []
        self.stderr_buffer = []
        
        # Command storage
        self.last_command = ""
        
        # Callbacks for progress updates
        self.progress_callbacks: List[Callable] = []
        
        # Latest metrics
        self.latest_metrics = {
            'pdr': None,
            'energy_consumed': None,
            'energy_residual': None,
            'throughput': None,
            'packets_sent': None,
            'packets_received': None
        }
        
    def add_progress_callback(self, callback: Callable):
        """Add a callback function for progress updates"""
        self.progress_callbacks.append(callback)
        
    def remove_progress_callback(self, callback: Callable):
        """Remove a progress callback"""
        if callback in self.progress_callbacks:
            self.progress_callbacks.remove(callback)
            
    def _emit_progress(self, event_type: str, data: Dict[str, Any]):
        """Emit progress event to all registered callbacks"""
        for callback in self.progress_callbacks:
            try:
                callback(event_type, data)
            except Exception as e:
                print(f"Error in progress callback: {e}")
                
    def detect_ns3_installation(self) -> Optional[str]:
        """
        Detect NS-3 installation using the same logic as run_all.py
        
        Returns:
            Path to NS-3 directory or None if not found
        """
        # Import the detection function from run_all.py
        sys.path.insert(0, str(self.scripts_dir))
        try:
            from run_all import find_ns3_installation
            ns3_path = find_ns3_installation()
            return ns3_path
        except Exception as e:
            print(f"Error detecting NS-3: {e}")
            return None
        finally:
            sys.path.pop(0)
            
    def start(self, parameters: Dict[str, Any]) -> bool:
        """
        Start simulation with given parameters
        
        Args:
            parameters: Dictionary of simulation parameters
            
        Returns:
            Boolean indicating if simulation started successfully
        """
        if self.is_running:
            return False
            
        # Generate session ID
        self.session_id = parameters.get('session_id', 
                                         datetime.now().strftime('%Y%m%d_%H%M%S'))
        
        # Calculate total runs
        protocols = parameters.get('protocols', ['GAODV'])
        runs = parameters.get('runs', 5)
        self.total_runs = len(protocols) * runs
        self.current_run = 0
        
        # Build command BEFORE starting thread so it's available immediately
        ns3_path = parameters.get('ns3_path') or self.detect_ns3_installation()
        if ns3_path:
            cmd = self._build_command(ns3_path, parameters)
            self.last_command = ' '.join(cmd)
        else:
            self.last_command = "NS-3 installation not found"
        
        # Start simulation in background thread
        self.is_running = True
        self.start_time = time.time()
        self.thread = threading.Thread(target=self._run_simulation, args=(parameters,))
        self.thread.daemon = True
        self.thread.start()
        
        return True
        
    def _run_simulation(self, parameters: Dict[str, Any]):
        """Execute simulation ONCE via run_all.py with all protocols"""
        try:
            # Detect NS-3 installation
            ns3_path = parameters.get('ns3_path') or self.detect_ns3_installation()
            if not ns3_path:
                self._emit_progress('error', {'message': 'NS-3 installation not found'})
                self.is_running = False
                return
            
            # Build command (should already be built in start(), but rebuild for execution)
            cmd = self._build_command(ns3_path, parameters)
            
            # Execute run_all.py ONCE (it handles all protocols × runs internally)
            self._execute_simulation(cmd, parameters)
            
            # Simulation complete
            if self.is_running:  # Only emit complete if not stopped
                elapsed_time = time.time() - self.start_time
                self._emit_progress('complete', {
                    'session_id': self.session_id,
                    'elapsed_time': elapsed_time,
                    'total_runs': self.total_runs
                })
                
        except Exception as e:
            self._emit_progress('error', {'message': f'Simulation error: {str(e)}'})
        finally:
            self.is_running = False
            self.process = None
    
    def get_command(self) -> str:
        """Get the last executed command string"""
        return self.last_command
            
    def _build_command(self, ns3_path: str, params: Dict[str, Any]) -> List[str]:
        """Build ONE run_all.py command with ALL protocols"""
        
        # Use run_all.py which handles building and parameter validation
        run_all_script = self.scripts_dir / 'run_all.py'
        
        # Get all protocols from parameters
        protocols = params.get('protocols', ['GAODV'])
        
        # Build command with ALL protocols at once
        cmd = [
            'python3',
            '-u',  # Unbuffered output (critical for real-time updates)
            str(run_all_script),
            '--protocols',
            *protocols,  # All protocols as separate arguments
            '--runs', str(params.get('runs', 5)),
            '--nodes', str(params.get('nodes', 50)),
            '--sinks', str(params.get('sinks', 10)),
            '--simtime', str(params.get('sim_time', 500)),
            '--txpower', str(params.get('tx_power', 7.5)),
            '--txrange', str(params.get('tx_range', 150)),
            '--node-spacing', str(params.get('node_spacing', 50)),
            '--initial-energy', str(params.get('initial_energy', 20.0)),
            '--packet-size', str(params.get('packet_size', 512)),
            '--packet-rate', str(params.get('packet_rate', 4)),
            '--idle-current', str(params.get('idle_current', 0.0273)),
            '--rx-current', str(params.get('rx_current', 0.0313)),
            '--tx-current-base', str(params.get('tx_current_base', 0.010)),
            '--ns3-path', str(ns3_path),
        ]
        
        # Add optional mobility/area parameters if provided
        if 'area_width' in params:
            cmd.extend(['--area-width', str(params['area_width'])])
        if 'area_height' in params:
            cmd.extend(['--area-height', str(params['area_height'])])
        if 'min_speed' in params:
            cmd.extend(['--min-speed', str(params['min_speed'])])
        if 'max_speed' in params:
            cmd.extend(['--max-speed', str(params['max_speed'])])
        if 'pause_time' in params:
            cmd.extend(['--pause-time', str(params['pause_time'])])
        
        # Add GAODV-specific parameters (run_all.py will use them only for GAODV)
        if 'ethf' in params:
            cmd.extend(['--ethf', str(params['ethf'])])
        if 'max_range' in params:
            cmd.extend(['--max-range', str(params['max_range'])])
        
        # Dashboard assumes NS-3 is already built
        cmd.append('--skip-build')
            
        # FORCE verbose mode for real-time output in dashboard
        # Without this, run_all.py waits for ALL runs to complete before showing output
        cmd.append('--verbose')
        
        return cmd
        
    def _execute_simulation(self, cmd: List[str], params: Dict[str, Any]):
        """Execute run_all.py and parse output - NO protocol parameter needed"""
        
        protocols = params.get('protocols', ['GAODV'])
        total_runs = len(protocols) * params.get('runs', 5)
        self.total_runs = total_runs
        
        # Execute run_all.py command
        try:
            import os
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1,  # Line buffering
                cwd=str(self.scripts_dir),
                preexec_fn=os.setsid  # Create process group for proper cleanup
            )
            
            # Read output line by line in real-time
            for line in self.process.stdout:
                if not self.is_running:
                    # Force kill the process immediately
                    try:
                        self.process.kill()
                    except:
                        pass
                    break
                    
                self.stdout_buffer.append(line)
                
                # Parse for protocol/run tracking
                self._parse_output_line(line)
                
                # Emit console output
                self._emit_progress('console', {'line': line.rstrip()})
                
            # Wait for process to complete (with sufficient timeout for large simulations)
            # 150 nodes × 500s simulation can take 20-30 minutes per run
            # Total: 25 runs × 30 min = 750 min = 45000 seconds max
            if self.is_running:
                try:
                    self.process.wait(timeout=50000)  # ~14 hours max (very large simulations)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()
            
            # Check exit code
            if self.process.returncode != 0 and self.is_running:
                self._emit_progress('warning', {
                    'message': f'run_all.py exited with code {self.process.returncode}'
                })
                    
        except Exception as e:
            self._emit_progress('error', {'message': f'Execution error: {str(e)}'})
        finally:
            self.process = None
                
    def _parse_output_line(self, line: str):
        """Parse run_all.py output for metrics and progress"""
        
        # Detect which protocol is running
        # run_all.py output: "Running: GAODV | Nodes: 50 | Run: 3"
        protocol_match = re.search(r'Running:\s+(\w+)\s+\|.*Run:\s+(\d+)', line)
        if protocol_match:
            protocol = protocol_match.group(1)
            run_num = int(protocol_match.group(2))
            self.current_protocol = protocol
            self.current_run += 1
            
            self._emit_progress('run_progress', {
                'protocol': protocol,
                'run': run_num,
                'overall_run': self.current_run,
                'total_runs': self.total_runs,
                'progress_percent': (self.current_run / self.total_runs * 100) if self.total_runs > 0 else 0
            })
        
        # PDR pattern: "✓ PDR: 99.98%" or "PDR: 99.98%"
        pdr_match = re.search(r'PDR:\s*([\d.]+)%', line)
        if pdr_match:
            self.latest_metrics['pdr'] = float(pdr_match.group(1))
            self._emit_progress('metric', {
                'type': 'pdr',
                'value': self.latest_metrics['pdr']
            })
        
        # Energy patterns (from run_all.py summary output)
        # "Energy: 1000.5J" or "Total Consumed Energy: 1000.5J"
        energy_match = re.search(r'(?:Energy|Total Consumed Energy):\s*([\d.]+)\s*J', line)
        if energy_match:
            self.latest_metrics['energy_consumed'] = float(energy_match.group(1))
            self._emit_progress('metric', {
                'type': 'energy_consumed',
                'value': self.latest_metrics['energy_consumed']
            })
        
        residual_match = re.search(r'Total Residual Energy:\s*([\d.]+)\s*J', line)
        if residual_match:
            self.latest_metrics['energy_residual'] = float(residual_match.group(1))
            self._emit_progress('metric', {
                'type': 'energy_residual',
                'value': self.latest_metrics['energy_residual']
            })
        
        # Throughput pattern: "Throughput: 20.48 kbps" or "Average Throughput: 20.48 kbps"
        throughput_match = re.search(r'(?:Average )?Throughput:\s*([\d.]+)\s*kbps', line)
        if throughput_match:
            self.latest_metrics['throughput'] = float(throughput_match.group(1))
            self._emit_progress('metric', {
                'type': 'throughput',
                'value': self.latest_metrics['throughput']
            })
        
        # Packets sent/received
        sent_match = re.search(r'Total Packets Sent:\s*(\d+)', line)
        if sent_match:
            self.latest_metrics['packets_sent'] = int(sent_match.group(1))
        
        received_match = re.search(r'Total Packets Received:\s*(\d+)', line)
        if received_match:
            self.latest_metrics['packets_received'] = int(received_match.group(1))
            
    def stop(self) -> bool:
        """Stop currently running simulation"""
        if not self.is_running:
            return False
            
        # Set flag to stop simulation loop
        self.is_running = False
        
        # Kill the current process if it exists
        if self.process and self.process.poll() is None:  # Process is still running
            try:
                import os
                import signal
                
                # Kill the entire process group (run_all.py + all its children including NS-3)
                # This ensures NS-3 simulation is actually terminated
                try:
                    os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
                    # Wait for graceful termination
                    try:
                        self.process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        # Force kill the entire process group
                        os.killpg(os.getpgid(self.process.pid), signal.SIGKILL)
                        self.process.wait()
                except ProcessLookupError:
                    # Process already dead
                    pass
                except Exception as e:
                    print(f"Error killing process group: {e}")
                    # Fallback: try killing just the parent process
                    try:
                        self.process.kill()
                        self.process.wait()
                    except:
                        pass
            except Exception as e:
                print(f"Error stopping process: {e}")
                # Try force kill as last resort
                try:
                    self.process.kill()
                except:
                    pass
                
        self._emit_progress('stopped', {
            'completed_runs': self.current_run,
            'total_runs': self.total_runs
        })
        
        return True
        
    def get_status(self) -> Dict[str, Any]:
        """Get current simulation status"""
        elapsed_time = time.time() - self.start_time if self.start_time else 0
        
        return {
            'is_running': self.is_running,
            'current_run': self.current_run,
            'total_runs': self.total_runs,
            'current_protocol': self.current_protocol,
            'session_id': self.session_id,
            'elapsed_time': elapsed_time,
            'progress_percent': (self.current_run / self.total_runs * 100) if self.total_runs > 0 else 0,
            'latest_metrics': self.latest_metrics.copy()
        }
        
    def get_results(self) -> List[Dict[str, Any]]:
        """Get list of result files for current session"""
        results = []
        
        if not self.results_dir.exists():
            return results
            
        # Find all CSV files for this session
        pattern = f'*_session_{self.session_id}.csv'
        for csv_file in self.results_dir.glob(pattern):
            results.append({
                'filename': csv_file.name,
                'path': str(csv_file),
                'size': csv_file.stat().st_size,
                'modified': csv_file.stat().st_mtime
            })
            
        return results
