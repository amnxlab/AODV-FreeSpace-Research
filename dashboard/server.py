"""
GAODV Dashboard Server
Flask application with WebSocket support for real-time simulation monitoring
"""

from flask import Flask, render_template, request, jsonify, send_file
from flask_socketio import SocketIO, emit
from flask_cors import CORS
import os
import sys
from pathlib import Path
from datetime import datetime
import json

# Add parent directory to path to import API modules
current_dir = Path(__file__).parent
project_root = current_dir.parent
sys.path.insert(0, str(project_root / 'scripts'))

from api.validators import validate_parameters, validate_ns3_path
from api.simulation import SimulationRunner
from api.visualizer import generate_charts, load_csv_data

# Initialize Flask app
app = Flask(__name__, 
            static_folder='static',
            static_url_path='/static',
            template_folder='public')
app.config['SECRET_KEY'] = 'gaodv-dashboard-secret-key-2026'
CORS(app)

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# Initialize simulation runner
simulation_runner = SimulationRunner(str(project_root))

# Results directory
RESULTS_DIR = project_root / 'results'
RESULTS_DIR.mkdir(exist_ok=True)


# ==================== HTTP Routes ====================

@app.route('/')
def index():
    """Serve main dashboard page"""
    return send_file('public/index.html')


@app.route('/api/parameters/defaults', methods=['GET'])
def get_default_parameters():
    """Return default simulation parameters"""
    defaults = {
        'nodes': 50,
        'sinks': 1,
        'node_spacing': 50.0,
        'area_width': 800.0,
        'area_height': 400.0,
        'tx_range': 150.0,
        'tx_power': 7.5,
        'initial_energy': 20.0,
        'idle_current': 0.0273,
        'rx_current': 0.0313,
        'tx_current_base': 0.010,
        'ethf': 0.1,
        'max_range': 250.0,
        'packet_size': 512,
        'packet_rate': 4,
        'sim_time': 500,
        'runs': 5,
        'protocols': ['GAODV'],
        'skip_build': False,
        'verbose': False
    }
    return jsonify(defaults)


@app.route('/api/parameters/validate', methods=['POST'])
def validate_params():
    """Validate simulation parameters"""
    params = request.json
    is_valid, errors, warnings = validate_parameters(params)
    
    return jsonify({
        'valid': is_valid,
        'errors': errors,
        'warnings': warnings
    })


@app.route('/api/simulation/start', methods=['POST'])
def start_simulation():
    """Start a new simulation"""
    params = request.json
    
    # Validate parameters
    is_valid, errors, warnings = validate_parameters(params)
    
    if not is_valid:
        return jsonify({
            'success': False,
            'errors': errors
        }), 400
        
    # Check if simulation is already running
    if simulation_runner.is_running:
        return jsonify({
            'success': False,
            'errors': ['A simulation is already running']
        }), 400
        
    # Start simulation
    success = simulation_runner.start(params)
    
    if success:
        # Get the command that will be executed
        command = simulation_runner.get_command()
        
        return jsonify({
            'success': True,
            'session_id': simulation_runner.session_id,
            'command': command,
            'warnings': warnings
        })
    else:
        return jsonify({
            'success': False,
            'errors': ['Failed to start simulation']
        }), 500


@app.route('/api/simulation/stop', methods=['POST'])
def stop_simulation():
    """Stop running simulation"""
    success = simulation_runner.stop()
    
    return jsonify({
        'success': success
    })


@app.route('/api/simulation/status', methods=['GET'])
def get_status():
    """Get current simulation status"""
    status = simulation_runner.get_status()
    return jsonify(status)


@app.route('/api/results/list', methods=['GET'])
def list_results():
    """List all result files organized by session"""
    results = {}
    
    if not RESULTS_DIR.exists():
        return jsonify({'sessions': {}})
        
    # Scan all CSV files
    for csv_file in sorted(RESULTS_DIR.glob('*.csv'), key=lambda x: x.stat().st_mtime, reverse=True):
        filename = csv_file.name
        
        # Extract session ID from filename: results_PROTOCOL_session_SESSIONID.csv
        parts = filename.split('_')
        if len(parts) >= 4 and parts[2] == 'session':
            protocol = parts[1]
            session_id = parts[3].replace('.csv', '')
            
            if session_id not in results:
                results[session_id] = {
                    'session_id': session_id,
                    'files': [],
                    'timestamp': session_id  # Session ID is timestamp
                }
                
            results[session_id]['files'].append({
                'filename': filename,
                'protocol': protocol,
                'size': csv_file.stat().st_size,
                'modified': datetime.fromtimestamp(csv_file.stat().st_mtime).isoformat()
            })
            
    return jsonify({
        'sessions': results
    })


@app.route('/api/results/csv/<filename>', methods=['GET'])
def get_csv_file(filename):
    """Get CSV file content"""
    csv_path = RESULTS_DIR / filename
    
    if not csv_path.exists():
        return jsonify({'error': 'File not found'}), 404
        
    # Return as download or JSON based on query parameter
    format_type = request.args.get('format', 'download')
    
    if format_type == 'json':
        data = load_csv_data(str(csv_path))
        return jsonify(data)
    else:
        return send_file(csv_path, as_attachment=True)


@app.route('/api/results/visualize/<filename>', methods=['GET'])
def visualize_results(filename):
    """Generate visualizations for a result file"""
    csv_path = RESULTS_DIR / filename
    
    if not csv_path.exists():
        return jsonify({'error': 'File not found'}), 404
        
    chart_types = request.args.getlist('charts')
    if not chart_types:
        chart_types = None  # Generate all
        
    charts = generate_charts(str(csv_path), chart_types)
    return jsonify(charts)


@app.route('/api/ns3/detect', methods=['GET'])
def detect_ns3():
    """Detect NS-3 installation"""
    ns3_path = simulation_runner.detect_ns3_installation()
    
    if ns3_path:
        return jsonify({
            'found': True,
            'path': ns3_path
        })
    else:
        return jsonify({
            'found': False,
            'message': 'NS-3 installation not found. Please install NS-3 or provide path manually.'
        })


@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.now().isoformat(),
        'version': '1.0.0'
    })


# ==================== WebSocket Events ====================

def simulation_progress_handler(event_type, data):
    """Handle simulation progress events and emit via WebSocket"""
    socketio.emit('simulation_event', {
        'type': event_type,
        'data': data,
        'timestamp': datetime.now().isoformat()
    }, namespace='/')


@socketio.on('connect')
def handle_connect():
    """Handle client connection"""
    print(f'Client connected: {request.sid}')
    emit('connected', {'message': 'Connected to GAODV Dashboard'})
    
    # Send current status
    status = simulation_runner.get_status()
    emit('status_update', status)


@socketio.on('disconnect')
def handle_disconnect():
    """Handle client disconnection"""
    print(f'Client disconnected: {request.sid}')


@socketio.on('subscribe_progress')
def handle_subscribe():
    """Subscribe to simulation progress updates"""
    # Check if callback already exists before adding to prevent duplicates
    if simulation_progress_handler not in simulation_runner.progress_callbacks:
        simulation_runner.add_progress_callback(simulation_progress_handler)
    emit('subscribed', {'message': 'Subscribed to progress updates'})


@socketio.on('request_status')
def handle_status_request():
    """Handle status request from client"""
    status = simulation_runner.get_status()
    emit('status_update', status)


# ==================== Error Handlers ====================

@app.errorhandler(404)
def not_found(error):
    """Handle 404 errors"""
    return jsonify({'error': 'Not found'}), 404


@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors"""
    return jsonify({'error': 'Internal server error'}), 500


# ==================== Main ====================

def run_server(host='0.0.0.0', port=5000, debug=False):
    """Run the Flask server with SocketIO"""
    print(f"""
╔══════════════════════════════════════════════════════════════╗
║          🌐 GAODV DASHBOARD SERVER STARTING                  ║
╠══════════════════════════════════════════════════════════════╣
║  Host:              {host:<43} ║
║  Port:              {port:<43} ║
║  Debug Mode:        {str(debug):<43} ║
║  Project Root:      {str(project_root)[:43]:<43} ║
╠══════════════════════════════════════════════════════════════╣
║  Access URL:        http://{host}:{port:<33} ║
╚══════════════════════════════════════════════════════════════╝
""")
    
    # Detect NS-3 installation
    ns3_path = simulation_runner.detect_ns3_installation()
    if ns3_path:
        print(f"✅ NS-3 detected at: {ns3_path}\n")
    else:
        print("⚠️  NS-3 installation not found - simulations will fail!\n")
    
    socketio.run(app, host=host, port=port, debug=debug, allow_unsafe_werkzeug=True)


if __name__ == '__main__':
    run_server(debug=True)
