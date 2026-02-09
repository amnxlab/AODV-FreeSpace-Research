"""
Parameter Validation Module for GAODV Simulation Dashboard
Validates all simulation parameters according to project constraints
"""

import re
from typing import Dict, List, Tuple, Any


class ValidationError(Exception):
    """Custom exception for validation errors"""
    pass


def validate_parameters(params: Dict[str, Any]) -> Tuple[bool, List[str], List[str]]:
    """
    Validate all simulation parameters
    
    Args:
        params: Dictionary of parameter name -> value
        
    Returns:
        Tuple of (is_valid, errors, warnings)
        - is_valid: Boolean indicating if parameters are valid
        - errors: List of error messages or dicts with 'message' and 'field'
        - warnings: List of warning messages or dicts with 'message' and 'field'
    """
    errors = []
    warnings = []
    
    def add_error(message: str, field: str = None):
        """Add error with optional field identifier"""
        if field:
            errors.append({'message': message, 'field': field})
        else:
            errors.append(message)
    
    def add_warning(message: str, field: str = None):
        """Add warning with optional field identifier"""
        if field:
            warnings.append({'message': message, 'field': field})
        else:
            warnings.append(message)
    
    # Network Topology Parameters
    nodes = params.get('nodes', 50)
    sinks = params.get('sinks', 10)
    mobility_model = params.get('mobility_model', 'STATIC_GRID')
    node_spacing = params.get('node_spacing', 50.0)
    area_width = params.get('area_width', 800.0)
    area_height = params.get('area_height', 400.0)
    min_speed = params.get('min_speed', 5.0)
    max_speed = params.get('max_speed', 55.0)
    pause_time = params.get('pause_time', 50.0)
    
    # RF Parameters
    tx_range = params.get('tx_range', 150.0)
    tx_power = params.get('tx_power', 7.5)
    
    # Energy Parameters (Phase 0 defaults)
    initial_energy = params.get('initial_energy', 20.0)
    idle_current = params.get('idle_current', 0.002)      # 2mA
    rx_current = params.get('rx_current', 0.0313)
    tx_current_base = params.get('tx_current_base', 0.180)  # 180mA
    ethf = params.get('ethf', 0.2)
    
    # Traffic Parameters
    packet_size = params.get('packet_size', 512)
    packet_rate = params.get('packet_rate', 4)
    
    # Simulation Control
    sim_time = params.get('sim_time', 500)
    runs = params.get('runs', 5)
    protocols = params.get('protocols', ['GAODV'])
    
    # Validate nodes
    if not isinstance(nodes, int) or nodes < 1:
        add_error("Nodes must be a positive integer (minimum 1)", "nodes")
    elif nodes > 1000:
        add_warning(f"Large node count ({nodes}) may result in very long simulation times", "nodes")
    
    # Validate sinks
    if not isinstance(sinks, int) or sinks < 1:
        add_error("Sinks must be a positive integer (minimum 1)", "sinks")
    elif sinks > nodes:
        add_error(f"Sinks ({sinks}) cannot exceed total nodes ({nodes})", "sinks")
    
    # Validate mobility model
    valid_mobility_models = ['STATIC_GRID', 'RANDOM_STATIC', 'RANDOM_DYNAMIC']
    if mobility_model not in valid_mobility_models:
        add_error(
            f"Invalid mobility model: {mobility_model}. Must be one of: {', '.join(valid_mobility_models)}",
            "mobility_model"
        )
    
    # Validate node spacing (only for STATIC_GRID)
    if mobility_model == 'STATIC_GRID':
        if node_spacing <= 0:
            add_error("Node spacing must be positive", "node_spacing")
        elif node_spacing > 200:
            add_warning(f"Large node spacing ({node_spacing}m) may result in disconnected network", "node_spacing")
        
        # CRITICAL: TX Range must exceed node spacing for grid topology
        if tx_range <= node_spacing:
            add_error(
                f"TX Range ({tx_range}m) MUST be greater than Node Spacing ({node_spacing}m) "
                f"for network connectivity. Current ratio: {tx_range/node_spacing:.2f}x",
                "tx_range"
            )
        elif tx_range < node_spacing * 1.5:
            add_warning(
                f"TX Range ({tx_range}m) is only {tx_range/node_spacing:.2f}x Node Spacing. "
                f"Recommend at least 2x for reliable connectivity.",
                "tx_range"
            )
    
    # Validate area dimensions (for random topologies)
    if mobility_model in ['RANDOM_STATIC', 'RANDOM_DYNAMIC']:
        if area_width <= 0 or area_height <= 0:
            add_error("Area dimensions must be positive for random topology", "area_width")
        
        # Check node density for random topologies
        area = area_width * area_height
        node_density = nodes / area
        if node_density < 0.0001:  # Less than 1 node per 10,000 m²
            add_warning(
                f"Low node density ({nodes} nodes in {area:.0f} m²). Network may be disconnected.",
                "area_width"
            )
    
    # Validate mobility parameters (only for RANDOM_DYNAMIC)
    if mobility_model == 'RANDOM_DYNAMIC':
        if min_speed < 0 or max_speed < 0:
            add_error("Speed values must be non-negative", "min_speed")
        elif min_speed > max_speed:
            add_error(
                f"Min speed ({min_speed} m/s) cannot exceed max speed ({max_speed} m/s)",
                "min_speed"
            )
        
        if pause_time < 0:
            add_error("Pause time must be non-negative", "pause_time")
        elif pause_time > sim_time / 2:
            add_warning(
                f"Pause time ({pause_time}s) is more than half of simulation time ({sim_time}s). "
                f"Nodes may not move much.",
                "pause_time"
            )
    
    # Validate TX power
    if not (0 <= tx_power <= 20):
        add_error("TX Power must be between 0 and 20 dBm", "tx_power")
    
    # Validate energy parameters
    if initial_energy <= 0:
        add_error("Initial energy must be positive", "initial_energy")
    
    if idle_current < 0 or rx_current < 0 or tx_current_base < 0:
        add_error("Current values must be non-negative", "idle_current")
    
    if not (0 <= ethf <= 1):
        add_error("ETHF (Energy Threshold Factor) must be between 0 and 1", "ethf")
    
    # Validate max_range
    max_range = params.get('max_range', 250.0)
    if max_range <= 0:
        add_error("Max Range must be positive", "max_range")
    elif max_range > 1000:
        add_warning(f"Very large max range ({max_range}m) may be unrealistic for WiFi", "max_range")
    
    # Battery life calculation and warning
    supply_voltage = 3.3  # Fixed in implementation
    if idle_current > 0:
        # IDLE-ONLY estimate (unrealistic - actual will be much shorter with RX/TX)
        idle_only_lifetime = initial_energy / (idle_current * supply_voltage)
        
        # More realistic estimate: weighted average assuming traffic conditions
        # Phase 0 duty cycle: 40% idle (2mA), 35% RX (31.3mA), 25% TX (180mA)
        rx_current = params.get('rx_current', 0.0313)
        tx_current_base = params.get('tx_current_base', 0.180)  # Phase 0: 180mA
        # TX current with power amplifier overhead
        tx_current_avg = tx_current_base * 1.2
        
        avg_current = (0.40 * idle_current + 0.35 * rx_current + 0.25 * tx_current_avg)
        realistic_lifetime = initial_energy / (avg_current * supply_voltage)
        
        # Only warn if simulation time significantly exceeds battery lifetime
        if sim_time > realistic_lifetime * 1.1:  # 10% tolerance
            min_energy_required = sim_time * avg_current * supply_voltage * 1.05
            add_warning(
                f"Simulation time ({sim_time}s) may exceed node lifetime "
                f"(~{realistic_lifetime:.0f}s with RX/TX activity). "
                f"Consider increasing initial energy to {min_energy_required:.1f}J for safety margin.",
                "initial_energy"
            )
    
    # Validate packet parameters
    if packet_size <= 0:
        add_error("Packet size must be positive", "packet_size")
    elif packet_size > 1500:
        add_warning(f"Large packet size ({packet_size} bytes) may cause fragmentation", "packet_size")
    
    if packet_rate <= 0:
        add_error("Packet rate must be positive", "packet_rate")
    elif packet_rate > 100:
        add_warning(f"High packet rate ({packet_rate} pkt/s) may saturate network", "packet_rate")
    
    # Calculate and validate data rate
    data_rate_bps = packet_rate * packet_size * 8
    if data_rate_bps > 1_000_000:  # 1 Mbps
        add_warning(
            f"High data rate ({data_rate_bps/1000:.1f} kbps) may exceed WiFi capacity",
            "packet_rate"
        )
    
    # Validate simulation time
    if sim_time <= 0:
        add_error("Simulation time must be positive", "sim_time")
    elif sim_time < 100:
        add_warning("Short simulation time (<100s) may not provide stable metrics", "sim_time")
    
    # Validate runs
    if not isinstance(runs, int) or runs < 1:
        add_error("Runs must be a positive integer (minimum 1)", "runs")
    
    # Validate protocols
    valid_protocols = ['GAODV', 'AODV', 'OLSR', 'DSDV', 'DSR']
    if not protocols or not isinstance(protocols, list):
        add_error("At least one protocol must be selected", "protocols")
    else:
        invalid = [p for p in protocols if p not in valid_protocols]
        if invalid:
            add_error(
                f"Invalid protocols: {', '.join(invalid)}. Valid: {', '.join(valid_protocols)}",
                "protocols"
            )
        
        # Validate GAODV-specific parameters
        has_gaodv = 'GAODV' in protocols
        
        if not has_gaodv and ('ethf' in params or 'max_range' in params):
            add_warning(
                "ETHF and Max Range parameters are GAODV-specific and will be ignored "
                "since GAODV protocol is not selected",
                "ethf"
            )
    
    is_valid = len(errors) == 0
    return is_valid, errors, warnings


def validate_ns3_path(path: str) -> bool:
    """
    Validate that the provided path contains a valid NS-3 installation
    
    Args:
        path: Path to NS-3 directory
        
    Returns:
        Boolean indicating if path is valid
    """
    import os
    
    if not os.path.exists(path):
        return False
    
    # Check for ns3 or waf executable
    ns3_exec = os.path.join(path, 'ns3')
    waf_exec = os.path.join(path, 'waf')
    
    return os.path.exists(ns3_exec) or os.path.exists(waf_exec)


def sanitize_session_id(session_id: str) -> str:
    """
    Sanitize session ID to prevent directory traversal attacks
    
    Args:
        session_id: User-provided session ID
        
    Returns:
        Sanitized session ID safe for file system use
    """
    # Remove any path separators and special characters
    sanitized = re.sub(r'[^a-zA-Z0-9_-]', '_', session_id)
    
    # Limit length
    if len(sanitized) > 50:
        sanitized = sanitized[:50]
    
    return sanitized
