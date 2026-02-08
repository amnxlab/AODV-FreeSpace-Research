#!/usr/bin/env python3
"""
GAODV Dashboard - One-click launcher
Opens the dashboard in your default browser.
"""
import subprocess
import webbrowser
import time
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.resolve()
SERVER_SCRIPT = SCRIPT_DIR / "server.py"

def main():
    print("╔══════════════════════════════════════════════════════════╗")
    print("║       🚀 GAODV Simulation Dashboard Launcher             ║")
    print("╚══════════════════════════════════════════════════════════╝")
    print()
    
    # Start the server
    print("Starting server...")
    server = subprocess.Popen(
        [sys.executable, str(SERVER_SCRIPT)],
        cwd=SCRIPT_DIR
    )
    
    # Wait for server to start
    time.sleep(1.5)
    
    # Open browser
    url = "http://localhost:8000"
    print(f"Opening browser: {url}")
    webbrowser.open(url)
    
    print()
    print("Dashboard is running!")
    print("Press Ctrl+C to stop the server.")
    print()
    
    try:
        server.wait()
    except KeyboardInterrupt:
        print("\nShutting down...")
        server.terminate()
        server.wait()
        print("✓ Server stopped")

if __name__ == '__main__':
    main()
