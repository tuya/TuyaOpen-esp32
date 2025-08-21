#!/usr/bin/env python3

"""
CI-friendly ESP-IDF installation script
This script reduces log noise by filtering progress output in CI environments
while preserving important installation messages.
"""

import os
import sys
import subprocess
import re


def is_ci_environment():
    """Check if running in a CI environment"""
    ci_vars = ['CI', 'GITHUB_ACTIONS', 'CONTINUOUS_INTEGRATION', 'BUILD_NUMBER']
    return any(os.getenv(var) for var in ci_vars)


def should_filter_line(line):
    """Determine if a line should be filtered out in CI mode"""
    line = line.strip()
    
    # Skip empty lines
    if not line:
        return True
    
    # Skip pure percentage progress lines
    if re.match(r'^\d+%?$', line):
        return True
    
    # Skip download progress lines like "96%" or "100.0/100.0"
    if re.match(r'^\d+(\.\d+)?(%|/\d+(\.\d+)?)?$', line):
        return True
    
    # Skip "Done" lines after downloads
    if line.lower() == 'done':
        return True
    
    # Skip "Destination:" lines
    if line.startswith('Destination:'):
        return True
    
    # Skip some verbose extraction messages
    if 'Extracting' in line and '.tar.' in line:
        return True
    
    return False


def run_install(idf_path, target):
    """Run the ESP-IDF installation"""
    install_script = os.path.join(idf_path, 'install.sh')
    
    if not os.path.exists(install_script):
        print(f"Error: install.sh not found at {install_script}")
        return 1
    
    cmd = [install_script, target]
    
    if is_ci_environment():
        print("CI environment detected, filtering output to reduce log noise...")
        
        # Run with output filtering
        process = subprocess.Popen(
            cmd,
            cwd=idf_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        important_lines = []
        
        for line in process.stdout:
            line = line.rstrip()
            
            if not should_filter_line(line):
                print(line)
                # Keep track of important lines
                if any(keyword in line.lower() for keyword in 
                       ['installing', 'downloading', 'error', 'fail', 'success', 'complete']):
                    important_lines.append(line)
        
        exit_code = process.wait()
        
        if important_lines:
            print("\n=== Installation Summary ===")
            for line in important_lines[-10:]:  # Show last 10 important lines
                print(line)
        
        print(f"\n=== Installation completed with exit code: {exit_code} ===")
        return exit_code
    
    else:
        print("Running in interactive mode with full output...")
        # Run normally with full output
        result = subprocess.run(cmd, cwd=idf_path)
        return result.returncode


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 silent_install.py <idf_path> <target>")
        sys.exit(1)
    
    idf_path = sys.argv[1]
    target = sys.argv[2]
    
    if not os.path.exists(idf_path):
        print(f"Error: IDF path does not exist: {idf_path}")
        sys.exit(1)
    
    exit_code = run_install(idf_path, target)
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
