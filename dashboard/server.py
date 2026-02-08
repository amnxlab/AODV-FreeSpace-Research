#!/usr/bin/env python3
"""
GAODV Dashboard - Combined Server
Serves both static files AND API on port 8000.
"""
import http.server
import json
import os
import subprocess
import threading
import urllib.parse
import mimetypes
from pathlib import Path

PORT = 8000
SCRIPT_DIR = Path(__file__).parent.resolve()
PUBLIC_DIR = SCRIPT_DIR / "public"
PROJECT_DIR = SCRIPT_DIR.parent
RUN_ALL = PROJECT_DIR / "scripts" / "run_all.py"
RESULTS_DIR = PROJECT_DIR / "results"

# === Global State ===
class State:
    running = False
    output = []
    process = None
    last_params = {}
    session_valid = False
    lock = threading.Lock()

state = State()

# Background thread to run simulation
def run_simulation(cmd):
    """Run the simulation command and capture output."""
    global state
    try:
        state.process = subprocess.Popen(
            cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=PROJECT_DIR,
            bufsize=1
        )
        
        for line in iter(state.process.stdout.readline, ''):
            with state.lock:
                state.output.append(line.rstrip())
        
        state.process.wait()
        
        with state.lock:
            exit_code = state.process.returncode
            if exit_code == 0:
                state.output.append("─" * 50)
                state.output.append("✓ Simulation completed successfully!")
            else:
                state.output.append(f"✗ Simulation exited with code {exit_code}")
            state.running = False
            state.process = None
            
    except Exception as e:
        with state.lock:
            state.output.append(f"ERROR: {str(e)}")
            state.running = False
            state.process = None


class CombinedHandler(http.server.BaseHTTPRequestHandler):
    """Handles both API requests and static file serving."""
    
    def log_message(self, *args):
        pass  # Suppress request logging
    
    def send_json(self, data, status=200):
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())
    
    def serve_static_file(self, file_path):
        """Serve a static file from PUBLIC_DIR."""
        try:
            with open(file_path, 'rb') as f:
                content = f.read()
            
            mime_type, _ = mimetypes.guess_type(str(file_path))
            mime_type = mime_type or 'application/octet-stream'
            
            self.send_response(200)
            self.send_header('Content-Type', mime_type)
            self.send_header('Content-Length', len(content))
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            self.wfile.write(content)
        except FileNotFoundError:
            self.send_error(404, 'File not found')
    
    def do_OPTIONS(self):
        self.send_json({})
    
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)
        
        # API Endpoints
        if path == '/status':
            with state.lock:
                self.send_json({
                    'running': state.running,
                    'params': state.last_params
                })
        
        elif path == '/output':
            since = int(query.get('since', [0])[0])
            with state.lock:
                lines = state.output[since:]
                self.send_json({
                    'lines': lines,
                    'total': len(state.output),
                    'running': state.running
                })
        
        elif path == '/folders':
            # List result folders/files
            folders = []
            if RESULTS_DIR.exists():
                for item in sorted(RESULTS_DIR.iterdir(), reverse=True):
                    if item.is_file() and item.suffix == '.csv':
                        folders.append({
                            'name': item.name,
                            'type': 'file',
                            'size': item.stat().st_size,
                            'modified': item.stat().st_mtime
                        })
            self.send_json({'folders': folders})
        
        elif path == '/graphs':
            # List graph images
            graphs_dir = PROJECT_DIR / "graphs"
            folder = query.get('folder', [''])[0]
            images = []
            
            target_dir = graphs_dir / folder if folder else graphs_dir
            if target_dir.exists():
                for item in sorted(target_dir.iterdir()):
                    if item.suffix.lower() in ['.png', '.jpg', '.jpeg', '.svg']:
                        images.append({
                            'name': item.name,
                            'path': f'/graph/{folder}/{item.name}' if folder else f'/graph/{item.name}'
                        })
            self.send_json({'images': images})
        
        elif path.startswith('/graph/'):
            # Serve graph image
            rel_path = path[7:]  # Remove '/graph/'
            file_path = PROJECT_DIR / "graphs" / rel_path
            if file_path.exists() and file_path.is_file():
                self.serve_static_file(file_path)
            else:
                self.send_error(404, 'Graph not found')
        
        elif path.startswith('/result/'):
            # Serve result file
            rel_path = path[8:]  # Remove '/result/'
            file_path = RESULTS_DIR / rel_path
            if file_path.exists() and file_path.is_file():
                self.serve_static_file(file_path)
            else:
                self.send_error(404, 'Result not found')
        
        # Static files
        else:
            if path == '/':
                path = '/index.html'
            
            file_path = PUBLIC_DIR / path.lstrip('/')
            
            if file_path.exists() and file_path.is_file():
                self.serve_static_file(file_path)
            else:
                self.send_error(404, 'Not found')
    
    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        
        if path == '/run':
            # Get POST data
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            
            try:
                params = json.loads(body) if body else {}
            except json.JSONDecodeError:
                self.send_json({'error': 'Invalid JSON'}, 400)
                return
            
            with state.lock:
                if state.running:
                    self.send_json({'error': 'Simulation already running'}, 409)
                    return
                
                # Build command from params
                cmd = f"python3 {RUN_ALL}"
                
                # Protocols
                protocols = params.get('protocols', ['GAODV'])
                if protocols:
                    cmd += f" --protocols {' '.join(protocols)}"
                
                # Core parameters
                if params.get('nodes'):
                    cmd += f" --nodes {params['nodes']}"
                if params.get('runs'):
                    cmd += f" --runs {params['runs']}"
                if params.get('sinks'):
                    cmd += f" --sinks {params['sinks']}"
                
                # RF/Wireless parameters
                if params.get('txRange'):
                    cmd += f" --txrange {params['txRange']}"
                if params.get('nodeSpacing'):
                    cmd += f" --node-spacing {params['nodeSpacing']}"
                if params.get('txPower'):
                    cmd += f" --txpower {params['txPower']}"
                
                # Energy parameters
                if params.get('initialEnergy'):
                    cmd += f" --initial-energy {params['initialEnergy']}"
                if params.get('idleCurrent'):
                    cmd += f" --idle-current {params['idleCurrent']}"
                if params.get('rxCurrent'):
                    cmd += f" --rx-current {params['rxCurrent']}"
                if params.get('txCurrentBase'):
                    cmd += f" --tx-current-base {params['txCurrentBase']}"
                
                # Simulation parameters
                if params.get('simtime'):
                    cmd += f" --simtime {params['simtime']}"
                if params.get('packetSize'):
                    cmd += f" --packet-size {params['packetSize']}"
                if params.get('packetRate'):
                    cmd += f" --packet-rate {params['packetRate']}"
                if params.get('ethf'):
                    cmd += f" --ethf {params['ethf']}"
                
                # Flags
                if params.get('skipBuild'):
                    cmd += " --skip-build"
                if params.get('noCollect'):
                    cmd += " --no-collect"
                
                # Reset state
                state.output = []
                state.output.append("─" * 50)
                state.output.append("▶ Starting GAODV Simulation")
                state.output.append("─" * 50)
                state.output.append(f"Command: {cmd}")
                state.output.append("")
                state.running = True
                state.last_params = params
                
                # Start simulation in background thread
                thread = threading.Thread(target=run_simulation, args=(cmd,))
                thread.daemon = True
                thread.start()
                
                self.send_json({'status': 'started', 'command': cmd})
        
        elif path == '/stop':
            with state.lock:
                if state.process:
                    try:
                        state.process.terminate()
                        state.output.append("")
                        state.output.append("⏹ Simulation aborted by user")
                        state.running = False
                        self.send_json({'status': 'stopped'})
                    except Exception as e:
                        self.send_json({'error': str(e)}, 500)
                else:
                    self.send_json({'error': 'No simulation running'}, 400)
        
        else:
            self.send_error(404, 'Endpoint not found')


if __name__ == '__main__':
    from http.server import ThreadingHTTPServer
    
    # Ensure directories exist
    PUBLIC_DIR.mkdir(parents=True, exist_ok=True)
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    
    server = ThreadingHTTPServer(('0.0.0.0', PORT), CombinedHandler)
    print(f"╔══════════════════════════════════════════════════════════╗")
    print(f"║  🚀 GAODV Dashboard running on http://localhost:{PORT}     ║")
    print(f"╠══════════════════════════════════════════════════════════╣")
    print(f"║  Serving: {str(PUBLIC_DIR)[:45]:<45} ║")
    print(f"║  API: /status, /output, /run, /stop, /folders, /graphs   ║")
    print(f"╚══════════════════════════════════════════════════════════╝")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n✓ Server stopped")
