#!/usr/bin/env python3
"""
GAODV Dashboard Launcher
Start the web dashboard server with configuration options
"""

import argparse
import sys
import os
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
