# GAODV Dashboard

Web-based interface for the GAODV (Geometric & Energy-Aware AODV) routing protocol simulation project. Provides real-time simulation monitoring, parameter configuration, and result visualization through a modern browser interface.

## Features

- **🎛️ Interactive Parameter Configuration** - Configure all simulation parameters through a user-friendly web interface
- **📊 Real-Time Monitoring** - Live progress updates via WebSocket with metrics streaming
- **📈 Result Visualization** - Automatic chart generation for PDR, energy consumption, and efficiency metrics
- **📁 Session Management** - Browse and visualize historical simulation results organized by session
- **⚡ Live Progress Tracking** - See current run, protocol, elapsed time, and real-time metrics
- **🔍 Parameter Validation** - Real-time validation with helpful warnings and error messages
- **🌐 Responsive Design** - Professional MANET research aesthetic with mobile support

## Architecture

```
dashboard/
├── server.py              # Flask + SocketIO server
├── start.py               # Launcher script with startup checks
├── requirements.txt       # Python dependencies
├── api/                   # Backend modules
│   ├── validators.py      # Parameter validation logic
│   ├── simulation.py      # Simulation runner and controller
│   └── visualizer.py      # Chart generation (matplotlib)
├── public/                # Frontend
│   └── index.html         # Single-page application
└── static/                # Static assets
```

## Installation

### Prerequisites

1. **Python 3.8+** with pip
2. **NS-3 installation** (version 3.42 recommended)
3. **GAODV module** (should be in `../gaodv-v1/`)

### Install Dependencies

From the dashboard directory:

```bash
cd dashboard
pip install -r requirements.txt
```

Or from the project root:

```bash
pip install -r dashboard/requirements.txt
```

### Verify Installation

The launcher script performs automatic checks:

```bash
python dashboard/start.py
```

This will verify:
- ✅ Python dependencies installed
- ✅ NS-3 installation detected
- ✅ GAODV module present
- ✅ Results directory exists

## Usage

### Quick Start

```bash
# Start with default settings (localhost:5000)
python dashboard/start.py

# Start on specific port
python dashboard/start.py --port 8080

# Enable debug mode (with auto-reload)
python dashboard/start.py --debug

# Start on all network interfaces
python dashboard/start.py --host 0.0.0.0 --port 5000
```

### Access Dashboard

Navigate to: **http://localhost:5000**

### Workflow

1. **Configure Tab** - Set simulation parameters:
   - Network Topology (nodes, sinks, spacing, area)
   - RF Parameters (TX range, TX power)
   - Energy Model (initial energy, currents, ETHF)
   - Traffic Configuration (packet size, rate)
   - Simulation Control (duration, runs, protocols)

2. **Execute Tab** - Run simulations:
   - Click "Validate & Continue" in Configure tab
   - Click "Start Simulation" in Execute tab
   - Monitor real-time progress and metrics
   - View console output from NS-3

3. **Results Tab** - Browse and visualize results:
   - Results organized by session folders
   - Click "Visualize" to generate charts
   - Download CSV files for further analysis

## Parameters Reference

### Network Topology

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| Nodes | 50 | 1-1000 | Total wireless nodes |
| Sinks | 10 | 1-nodes | Receiver nodes |
| Node Spacing | 50.0 m | 10-200 m | Grid spacing |
| Area Width | 800.0 m | - | X-dimension of ground plane |
| Area Length | 400.0 m | - | Y-dimension of ground plane |

### RF Parameters

| Parameter | Default | Range | Constraint |
|-----------|---------|-------|------------|
| TX Range | 150.0 m | 50-500 m | Must exceed spacing |
| TX Power | 7.5 dBm | 0-20 dBm | - |

### Energy Model

| Parameter | Default | Unit | Description |
|-----------|---------|------|-------------|
| Initial Energy | 20.0 J | Joules | Per-node battery |
| Idle Current | 0.0273 A | Amps | 27.3 mA |
| RX Current | 0.0313 A | Amps | 31.3 mA |
| TX Current Base | 0.010 A | Amps | 10 mA |
| ETHF | 0.1 | 0-1 | Energy threshold |
| Max Range | 250.0 m | Meters | Maximum transmission range |

**Battery Life Calculation:**

The dashboard calculates two estimates:
1. **Idle-only lifetime (unrealistic upper bound):**
   ```
   Idle lifetime = Initial Energy / (Idle Current × 3.3V)
   Default: 20J / (0.0273A × 3.3V) ≈ 222 seconds
   ```

2. **Realistic lifetime with RX/TX activity (recommended):**
   ```
   Average Current = 0.40×Idle + 0.35×RX + 0.25×TX
   Realistic lifetime = Initial Energy / (Avg Current × 3.3V)
   Default: 20J / (0.0281A × 3.3V) ≈ 216 seconds
   ```

The realistic estimate assumes typical MANET duty cycle with 4 pkt/s traffic.
**Always use the realistic estimate** for planning simulation duration.
Note: Actual lifetime will be even shorter under heavy traffic or with higher TX power.

### Traffic Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| Packet Size | 512 bytes | Payload per packet |
| Packet Rate | 4 pkt/s | Transmission rate |

**Data Rate:** `packet_rate × packet_size × 8 = 16,384 bps`

### Simulation Control

| Parameter | Default | Options |
|-----------|---------|---------|
| Simulation Time | 500 s | Duration |
| Runs | 5 | Repetitions per protocol |
| Protocols | GAODV | GAODV, AODV, OLSR, DSDV, DSR |

## API Endpoints

### REST API

```http
GET  /api/parameters/defaults          # Get default parameters
POST /api/parameters/validate          # Validate parameters
POST /api/simulation/start             # Start simulation
POST /api/simulation/stop              # Stop running simulation
GET  /api/simulation/status            # Get current status
GET  /api/results/list                 # List all result sessions
GET  /api/results/csv/<filename>       # Download/view CSV file
GET  /api/results/visualize/<filename> # Generate charts
GET  /api/ns3/detect                   # Detect NS-3 installation
GET  /api/health                       # Health check
```

### WebSocket Events

**Server → Client:**
- `connected` - Connection established
- `simulation_event` - Progress updates (run_start, metric, console, complete, error)
- `status_update` - Full status object

**Client → Server:**
- `connect` - Establish connection
- `subscribe_progress` - Subscribe to progress events
- `request_status` - Request current status

## Result Files

### CSV Format

Files generated: `results_<PROTOCOL>_session_<TIMESTAMP>.csv`

Example: `results_GAODV_session_20260208_165628.csv`

**Columns:**
```csv
run,protocol,nodes,simtime,txPower,txRange,packetsSent,packetsReceived,
pdr,initialEnergy,consumedEnergy,residualEnergy
```

### Visualization Charts

The dashboard generates 4 chart types:

1. **PDR Comparison** - Bar chart of packet delivery ratio across protocols
2. **Energy Consumption** - Consumed vs residual energy
3. **Efficiency** - PDR vs Energy scatter plot
4. **Overview** - 4-panel dashboard with key metrics

## Troubleshooting

### NS-3 Not Found

```
⚠️  NS-3 installation not found
```

**Solutions:**
- Install NS-3 in standard location (`~/ns-3.42/`)
- Set `--ns3-path` parameter in simulation configuration
- Check NS-3 paths in `scripts/run_all.py` detection logic

### Port Already in Use

```
Error: Address already in use
```

**Solution:**
```bash
# Use a different port
python dashboard/start.py --port 8080
```

### Module Import Errors

```
ModuleNotFoundError: No module named 'flask'
```

**Solution:**
```bash
pip install -r dashboard/requirements.txt
```

### GAODV Module Missing

```
⚠️  GAODV module directory not found
```

**Solution:**
- Ensure `gaodv-v1/` directory exists in project root
- Directory should contain GAODV protocol implementation

### Simulation Fails to Start

**Check:**
1. NS-3 installation path is correct
2. Source file `manet-compare-freespace-v1.cc` exists in NS-3 scratch/
3. GAODV module is copied to NS-3 src/
4. Parameters pass validation (TX range > node spacing)

### Battery Life Exceeded

```
Error: Simulation time exceeds node lifetime
```

**Solution:**
- Increase `Initial Energy` parameter
- Reduce `Simulation Time`
- Adjust current draw parameters

## Development

### Running in Debug Mode

```bash
python dashboard/start.py --debug
```

Debug mode features:
- Auto-reload on file changes
- Detailed error messages
- Flask debugger enabled

### Project Structure

```python
# Backend modules
from api.validators import validate_parameters
from api.simulation import SimulationRunner  
from api.visualizer import generate_charts

# Server
from server import run_server
```

### Adding New Features

1. **Backend API** - Add endpoints in `server.py`
2. **Frontend UI** - Modify `public/index.html`
3. **Validation** - Update `api/validators.py`
4. **Charts** - Extend `api/visualizer.py`

## Performance Notes

### Execution Time Estimates

Rough estimate: **10 minutes per 500s simulation at 50 nodes**

Factors affecting time:
- Node count (linear scaling)
- Simulation duration (linear scaling)
- Protocol complexity (GAODV > AODV > OLSR)
- Number of runs (linear scaling)

**Example:**
- 50 nodes, 500s, 5 runs, 1 protocol = ~50 minutes
- 30 nodes, 100s, 3 runs, 2 protocols = ~12 minutes

### Resource Usage

- **CPU:** One core per NS-3 simulation (sequential execution)
- **Memory:** ~1GB RAM per simulation instance
- **Disk:** ~10KB per result CSV file

## Security Notes

⚠️ **Development Use Only**

This dashboard is designed for **local development and research use**. For production deployment:

1. Enable authentication (Flask-Login)
2. Use HTTPS (flask-tls or reverse proxy)
3. Restrict CORS origins
4. Sanitize all user inputs
5. Run behind firewall
6. Use production WSGI server (gunicorn)

## License

Same as parent project (AODV-FreeSpace-Research)

## Support

For issues related to:
- **Dashboard:** Check this README and troubleshooting section
- **GAODV Protocol:** See main project README.md
- **NS-3 Simulation:** Consult NS-3 documentation

## Credits

**GAODV Dashboard** - Web interface for NS-3 MANET simulations

Part of the AODV-FreeSpace-Research project implementing the EDA-AODV (Energy and Distance Aware AODV) protocol with geometric enhancements.

---

**Version:** 1.0.0  
**Last Updated:** February 8, 2026
