#!/usr/bin/env python3
"""
Test script to verify Windows build environment for OneSDK
"""

import os
import sys
import subprocess
import platform

def check_command(command, description):
    """Check if a command is available"""
    try:
        result = subprocess.run([command, '--version'], 
                              capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            version = result.stdout.strip().split('\n')[0]
            print(f"✓ {description}: {version}")
            return True
        else:
            print(f"✗ {description}: Command failed")
            return False
    except FileNotFoundError:
        print(f"✗ {description}: Not found")
        return False
    except subprocess.TimeoutExpired:
        print(f"✗ {description}: Timeout")
        return False

def check_environment():
    """Check the build environment"""
    print("=" * 60)
    print("OneSDK Windows Build Environment Check")
    print("=" * 60)
    
    # Check OS
    print(f"Operating System: {platform.system()} {platform.release()}")
    print(f"Architecture: {platform.machine()}")
    print(f"Python Version: {sys.version}")
    print()
    
    # Check required tools
    tools = [
        ('cmake', 'CMake'),
        ('git', 'Git'),
    ]
    
    all_tools_ok = True
    for cmd, desc in tools:
        if not check_command(cmd, desc):
            all_tools_ok = False
    
    print()
    
    # Check compilers
    print("Checking C++ Compilers:")
    compilers = [
        ('cl', 'Visual Studio MSVC'),
        ('gcc', 'MinGW-w64 GCC'),
    ]
    
    compiler_found = False
    for cmd, desc in compilers:
        if check_command(cmd, desc):
            compiler_found = True
    
    if not compiler_found:
        print("✗ No C++ compiler found!")
        all_tools_ok = False
    
    print()
    
    # Check environment variables
    print("Checking Environment Variables:")
    env_vars = [
        ('OPENSSL_ROOT_DIR', 'OpenSSL Root Directory'),
        ('WINDOWS_SDK_PATH', 'Windows SDK Path'),
    ]
    
    for var, desc in env_vars:
        value = os.environ.get(var)
        if value:
            print(f"✓ {desc}: {value}")
        else:
            print(f"⚠ {desc}: Not set (optional)")
    
    print()
    
    # Check project structure
    print("Checking Project Structure:")
    required_files = [
        'CMakeLists.txt',
        'build.bat',
        'build.ps1',
        'docs/develop_windows.md',
    ]
    
    for file in required_files:
        if os.path.exists(file):
            print(f"✓ {file}")
        else:
            print(f"✗ {file}: Missing")
            all_tools_ok = False
    
    print()
    
    # Summary
    print("=" * 60)
    if all_tools_ok and compiler_found:
        print("✓ Environment check PASSED")
        print("You should be able to build OneSDK on Windows")
        print()
        print("Next steps:")
        print("1. Configure your credentials in example files")
        print("2. Run: build.bat")
        print("3. Or run: .\\build.ps1")
    else:
        print("✗ Environment check FAILED")
        print("Please install missing tools and try again")
        print()
        print("See docs/develop_windows.md for detailed setup instructions")
    
    print("=" * 60)
    
    return all_tools_ok and compiler_found

def test_cmake_config():
    """Test CMake configuration"""
    print("\nTesting CMake Configuration:")
    
    if not os.path.exists('CMakeLists.txt'):
        print("✗ CMakeLists.txt not found")
        return False
    
    try:
        # Create build directory
        os.makedirs('build_test', exist_ok=True)
        
        # Run CMake configuration
        result = subprocess.run([
            'cmake', '-G', 'Visual Studio 17 2022', '..'
        ], cwd='build_test', capture_output=True, text=True, timeout=60)
        
        if result.returncode == 0:
            print("✓ CMake configuration successful")
            
            # Clean up
            import shutil
            shutil.rmtree('build_test')
            return True
        else:
            print("✗ CMake configuration failed:")
            print(result.stderr)
            return False
            
    except Exception as e:
        print(f"✗ CMake test failed: {e}")
        return False

if __name__ == "__main__":
    env_ok = check_environment()
    
    if env_ok:
        test_cmake_config() 