#!/usr/bin/env python3
"""
GAODV Dashboard Launcher
Start the web dashboard server with configuration options
"""

import argparse
import sys
import os
import subprocess
import signal
from pathlib import Path

# Add current directory to path
sys.path.insert(0, str(Path(__file__).parent))


def print_banner():
    """Print startup banner"""
    print("""
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║              🌐  GAODV DASHBOARD LAUNCHER  🌐                ║
║                                                              ║
║     Geometric & Energy-Aware AODV Routing Protocol          ║
║              NS-3 Simulation Web Interface                   ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
""")


def check_dependencies():
    """Check if required Python packages are installed"""
    print("🔍 Checking dependencies...")
    
    required = [
        'flask',
        'flask_socketio',
        'flask_cors',
        'pandas',
        'matplotlib',
        'numpy'
    ]
    
    missing = []
    
    for package in required:
        try:
            __import__(package)
            print(f"  ✅ {package}")
        except ImportError:
            print(f"  ❌ {package} - NOT FOUND")
            missing.append(package)
    
    if missing:
        print(f"\n⚠️  Missing dependencies: {', '.join(missing)}")
        print(f"\n📦 Install with: pip install -r requirements.txt")
        return False
    
    print("\n✅ All dependencies satisfied\n")
    return True


def check_ns3_installation(project_root):
    """Check for NS-3 installation"""
    print("🔍 Checking NS-3 installation...")
    
    sys.path.insert(0, str(project_root / 'scripts'))
    
    try:
        from run_all import find_ns3_installation
        ns3_path = find_ns3_installation()
        
        if ns3_path:
            print(f"  ✅ NS-3 found at: {ns3_path}\n")
            return True
        else:
            print("  ⚠️  NS-3 installation not found")
            print("  ℹ️  Simulations will fail without NS-3")
            print("  ℹ️  Install NS-3 or provide path via dashboard\n")
            return True  # Don't block startup
            
    except Exception as e:
        print(f"  ⚠️  Error detecting NS-3: {e}\n")
        return True  # Don't block startup


def check_gaodv_module(project_root):
    """Check for GAODV module"""
    print("🔍 Checking GAODV module...")
    
    gaodv_dir = project_root / 'gaodv-v1'
    
    if gaodv_dir.exists():
        print(f"  ✅ GAODV module found\n")
        return True
    else:
        print(f"  ⚠️  GAODV module directory not found")
        print(f"  Expected: {gaodv_dir}\n")
        return True  # Don't block startup


def setup_results_directory(project_root):
    """Ensure results directory exists"""
    results_dir = project_root / 'results'
    results_dir.mkdir(exist_ok=True)
    print(f"📁 Results directory: {results_dir}")


def kill_process_on_port(port):
    """Kill any process using the specified port"""
    try:
        # Try lsof first (works on Linux and macOS)
        result = subprocess.run(
            ['lsof', '-ti', f':{port}'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0 and result.stdout.strip():
            pids = result.stdout.strip().split('\n')
            for pid in pids:
                if pid:
                    try:
                        os.kill(int(pid), signal.SIGTERM)
                        print(f"  ✅ Killed process {pid} on port {port}")
                    except ProcessLookupError:
                        pass  # Process already gone
                    except PermissionError:
                        print(f"  ⚠️  Permission denied to kill process {pid}")
                        return False
            return True
        return True  # No process found, port is free
        
    except FileNotFoundError:
        # lsof not available, try fuser (Linux)
        try:
            result = subprocess.run(
                ['fuser', '-k', f'{port}/tcp'],
                capture_output=True,
                text=True,
                timeout=5
            )
            if 'killed' in result.stderr.lower() or result.returncode == 0:
                print(f"  ✅ Killed process on port {port}")
            return True
        except FileNotFoundError:
            # Neither lsof nor fuser available
            print(f"  ⚠️  Cannot automatically kill process on port {port}")
            print(f"  ℹ️  Please manually run: lsof -ti:{port} | xargs kill -9")
            return False
    except Exception as e:
        print(f"  ⚠️  Error checking port {port}: {e}")
        return False


def check_port_available(port):
    """Check if a port is available and try to free it if not"""
    import socket
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(1)
    
    try:
        sock.bind(('0.0.0.0', port))
        sock.close()
        return True  # Port is available
    except OSError:
        # Port is in use
        print(f"\n⚠️  Port {port} is already in use")
        print(f"🔧 Attempting to free port {port}...\n")
        
        if kill_process_on_port(port):
            # Wait a moment for port to be freed
            import time
            time.sleep(1)
            
            # Check again
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1)
                sock.bind(('0.0.0.0', port))
                sock.close()
                print(f"  ✅ Port {port} is now available\n")
                return True
            except OSError:
                print(f"  ❌ Port {port} still in use after cleanup attempt\n")
                return False
        else:
            return False
    finally:
        try:
            sock.close()
        except:
            pass


def main():
    """Main entry point"""
    # Parse arguments
    parser = argparse.ArgumentParser(
        description='GAODV Dashboard - Web interface for NS-3 MANET simulations',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Start with default settings
  python start.py
  
  # Start on specific port
  python start.py --port 8080
  
  # Start with debug mode
  python start.py --debug
  
  # Start on all interfaces
  python start.py --host 0.0.0.0 --port 5000
        """
    )
    
    parser.add_argument('--host', type=str, default='0.0.0.0',
                      help='Host to bind server (default: 0.0.0.0)')
    parser.add_argument('--port', type=int, default=5000,
                      help='Port to bind server (default: 5000)')
    parser.add_argument('--debug', action='store_true',
                      help='Enable debug mode')
    parser.add_argument('--no-checks', action='store_true',
                      help='Skip startup checks')
    
    args = parser.parse_args()
    
    # Print banner
    print_banner()
    
    # Get project root
    dashboard_dir = Path(__file__).parent
    project_root = dashboard_dir.parent
    
    print(f"📂 Project root: {project_root}\n")
    
    # Perform startup checks
    if not args.no_checks:
        if not check_dependencies():
            print("\n❌ Dependency check failed. Please install required packages.\n")
            sys.exit(1)
        
        check_ns3_installation(project_root)
        check_gaodv_module(project_root)
        setup_results_directory(project_root)
    
    # Check if port is available and free it if needed
    if not check_port_available(args.port):
        print(f"\n❌ Cannot start server - port {args.port} is in use")
        print(f"💡 Try using a different port: python start.py --port 8080\n")
        sys.exit(1)
    
    print("━" * 64)
    print()
    
    # Import and run server
    try:
        from server import run_server
        
        print(f"🚀 Starting dashboard server...")
        print(f"   Host: {args.host}")
        print(f"   Port: {args.port}")
        print(f"   Debug: {args.debug}")
        print()
        print(f"🌐 Access dashboard at: http://{args.host if args.host != '0.0.0.0' else 'localhost'}:{args.port}")
        print()
        print("Press Ctrl+C to stop the server")
        print("━" * 64)
        print()
        
        run_server(host=args.host, port=args.port, debug=args.debug)
        
    except KeyboardInterrupt:
        print("\n\n⏹️  Server stopped by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n\n❌ Error starting server: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
