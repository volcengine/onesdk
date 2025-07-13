#!/usr/bin/env python3
"""
Script to fix CMake version requirements in example CMakeLists.txt files
"""

import os
import re

def fix_cmake_version(file_path):
    """Fix CMake version requirement in a single file"""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already updated
    if 'cmake_minimum_required(VERSION 3.10)' in content:
        print(f"Skipping {file_path} - already updated")
        return False
    
    # Update cmake_minimum_required version
    content = re.sub(
        r'cmake_minimum_required\(VERSION 3\.[0-9]\)',
        'cmake_minimum_required(VERSION 3.10)',
        content
    )
    
    # Write back the updated content
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Updated {file_path}")
    return True

def fix_target_link_libraries_syntax(file_path):
    """Fix target_link_libraries syntax issues"""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Fix the syntax error where ${PLATFORM_LIBS} is on a separate line
    # Look for patterns like:
    # target_link_libraries(...)
    #             ${PLATFORM_LIBS})
    
    # This is a more complex fix that needs to be done manually for each file
    # For now, let's just check if the file has the problematic pattern
    if '${PLATFORM_LIBS})' in content and 'target_link_libraries' in content:
        print(f"Warning: {file_path} may have target_link_libraries syntax issues")
        return True
    
    return False

def main():
    """Main function to fix all example CMakeLists.txt files"""
    
    # Find all CMakeLists.txt files in examples directory
    examples_dir = "examples"
    cmake_files = []
    
    for root, dirs, files in os.walk(examples_dir):
        for file in files:
            if file == "CMakeLists.txt":
                cmake_files.append(os.path.join(root, file))
    
    print(f"Found {len(cmake_files)} CMakeLists.txt files to check")
    
    updated_count = 0
    syntax_issues = []
    
    for cmake_file in cmake_files:
        if fix_cmake_version(cmake_file):
            updated_count += 1
        
        if fix_target_link_libraries_syntax(cmake_file):
            syntax_issues.append(cmake_file)
    
    print(f"\nUpdated {updated_count} files successfully")
    
    if syntax_issues:
        print(f"\nWarning: {len(syntax_issues)} files may have syntax issues:")
        for file in syntax_issues:
            print(f"  - {file}")

if __name__ == "__main__":
    main() 