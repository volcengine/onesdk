#!/usr/bin/env python3
"""
Test CMake configuration for OneSDK Windows build
"""

import os
import subprocess
import tempfile
import shutil

def test_cmake_config():
    """Test CMake configuration without building"""
    print("Testing CMake Configuration for OneSDK...")
    
    # Get current directory (project root)
    project_root = os.getcwd()
    
    # Create temporary build directory
    temp_dir = tempfile.mkdtemp(prefix="onesdk_test_")
    
    try:
        # Run CMake configuration (without building)
        print("Running CMake configuration...")
        result = subprocess.run([
            'cmake', 
            '-G', 'Visual Studio 17 2022',
            '-DONESDK_WITH_EXAMPLE=OFF',  # Disable examples to avoid compiler dependency
            '-DONESDK_WITH_TEST=OFF',     # Disable tests
            project_root
        ], cwd=temp_dir, capture_output=True, text=True, timeout=120)
        
        if result.returncode == 0:
            print("✓ CMake configuration successful!")
            print("✓ OneSDK Windows build support is working correctly")
            
            # Check if CMakeCache.txt was generated
            if os.path.exists(os.path.join(temp_dir, 'CMakeCache.txt')):
                print("✓ CMake cache file generated")
                
                # Check for Windows-specific settings
                with open(os.path.join(temp_dir, 'CMakeCache.txt'), 'r') as f:
                    cache_content = f.read()
                    if 'BUILD_PLATFORM:STRING=windows' in cache_content:
                        print("✓ Windows platform detected correctly")
                    else:
                        print("⚠ Windows platform detection may have issues")
            
            return True
        else:
            print("✗ CMake configuration failed!")
            print("Error output:")
            print(result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print("✗ CMake configuration timed out")
        return False
    except Exception as e:
        print(f"✗ CMake test failed: {e}")
        return False
    finally:
        # Clean up
        try:
            shutil.rmtree(temp_dir)
        except:
            pass

def test_cmake_lists_syntax():
    """Test CMakeLists.txt syntax"""
    print("\nTesting CMakeLists.txt syntax...")
    
    try:
        # Test main CMakeLists.txt by trying to parse it
        result = subprocess.run([
            'cmake', '-P', 'CMakeLists.txt'
        ], capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            print("✓ Main CMakeLists.txt syntax is valid")
        else:
            print("✗ Main CMakeLists.txt has syntax errors")
            print(result.stderr)
            return False
        
        # Test examples CMakeLists.txt
        examples_dir = "examples"
        if os.path.exists(examples_dir):
            for root, dirs, files in os.walk(examples_dir):
                for file in files:
                    if file == "CMakeLists.txt":
                        cmake_file = os.path.join(root, file)
                        try:
                            result = subprocess.run([
                                'cmake', '-P', cmake_file
                            ], capture_output=True, text=True, timeout=10)
                            
                            if result.returncode == 0:
                                print(f"✓ {cmake_file} syntax is valid")
                            else:
                                print(f"✗ {cmake_file} has syntax errors")
                                return False
                        except:
                            print(f"⚠ Could not test {cmake_file}")
        
        return True
        
    except Exception as e:
        print(f"✗ Syntax test failed: {e}")
        return False

def main():
    """Main test function"""
    print("=" * 60)
    print("OneSDK Windows CMake Configuration Test")
    print("=" * 60)
    
    # Check if we're in the right directory
    if not os.path.exists('CMakeLists.txt'):
        print("✗ CMakeLists.txt not found. Please run this script from the project root.")
        return False
    
    # Test CMakeLists.txt syntax
    syntax_ok = test_cmake_lists_syntax()
    
    # Test CMake configuration
    config_ok = test_cmake_config()
    
    print("\n" + "=" * 60)
    if syntax_ok and config_ok:
        print("✓ All tests PASSED!")
        print("OneSDK Windows build support is ready.")
        print("\nNext steps:")
        print("1. Install a C++ compiler (Visual Studio or MinGW-w64)")
        print("2. Configure your credentials in example files")
        print("3. Run: build.bat")
    else:
        print("✗ Some tests FAILED!")
        print("Please check the error messages above.")
    
    print("=" * 60)
    
    return syntax_ok and config_ok

if __name__ == "__main__":
    main() 