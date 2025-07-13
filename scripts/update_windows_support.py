#!/usr/bin/env python3
"""
Script to update all example CMakeLists.txt files to support Windows platform
"""

import os
import re
import glob

def update_cmakelists_file(file_path):
    """Update a single CMakeLists.txt file to support Windows"""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already updated
    if 'BUILD_PLATFORM' in content:
        print(f"Skipping {file_path} - already updated")
        return False
    
    # Add platform detection after project declaration
    project_pattern = r'(project\([^)]+\)\s*C?\s*)'
    platform_detection = '''# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_PLATFORM "macos")
else()
    set(BUILD_PLATFORM "linux")
endif()

'''
    
    content = re.sub(project_pattern, r'\1\n' + platform_detection, content)
    
    # Replace hardcoded homebrew include with platform-specific includes
    homebrew_include = 'include_directories("/opt/homebrew/include") # openssl'
    platform_includes = '''# Platform-specific include directories
if(BUILD_PLATFORM STREQUAL "macos")
    include_directories("/opt/homebrew/include") # openssl
elseif(BUILD_PLATFORM STREQUAL "windows")
    # Windows specific include directories
    if(DEFINED ENV{OPENSSL_ROOT_DIR})
        include_directories("$ENV{OPENSSL_ROOT_DIR}/include")
    endif()
    if(DEFINED ENV{WINDOWS_SDK_PATH})
        include_directories("$ENV{WINDOWS_SDK_PATH}/Include")
    endif()
endif()'''
    
    content = content.replace(homebrew_include, platform_includes)
    
    # Add platform-specific libraries before target_link_libraries
    if 'target_link_libraries' in content:
        platform_libs = '''# Platform-specific libraries
if(BUILD_PLATFORM STREQUAL "windows")
    set(PLATFORM_LIBS ws2_32 crypt32 iphlpapi)
else()
    set(PLATFORM_LIBS "")
endif()

'''
        
        # Find the target_link_libraries line and add platform libs before it
        lines = content.split('\n')
        updated_lines = []
        added_platform_libs = False
        
        for line in lines:
            if 'target_link_libraries' in line and not added_platform_libs:
                updated_lines.append(platform_libs)
                added_platform_libs = True
            updated_lines.append(line)
        
        content = '\n'.join(updated_lines)
        
        # Update target_link_libraries to include platform libs
        content = re.sub(
            r'(target_link_libraries\([^)]+\)\s*$)',
            r'\1\n            ${PLATFORM_LIBS})',
            content,
            flags=re.MULTILINE
        )
    
    # Write back the updated content
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Updated {file_path}")
    return True

def main():
    """Main function to update all example CMakeLists.txt files"""
    
    # Find all CMakeLists.txt files in examples directory
    examples_dir = "examples"
    cmake_files = []
    
    for root, dirs, files in os.walk(examples_dir):
        for file in files:
            if file == "CMakeLists.txt":
                cmake_files.append(os.path.join(root, file))
    
    print(f"Found {len(cmake_files)} CMakeLists.txt files to update")
    
    updated_count = 0
    for cmake_file in cmake_files:
        if update_cmakelists_file(cmake_file):
            updated_count += 1
    
    print(f"\nUpdated {updated_count} files successfully")

if __name__ == "__main__":
    main() 