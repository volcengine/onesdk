#!/usr/bin/env python3
"""
Script to fix target_link_libraries syntax issues in CMakeLists.txt files
"""

import os
import re

def fix_target_link_libraries_syntax(file_path):
    """Fix target_link_libraries syntax issues in a file"""
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Fix the specific pattern where ${PLATFORM_LIBS}) is on a separate line
    # This is the main issue causing the parse error
    
    # Split into lines and process
    lines = content.split('\n')
    fixed_lines = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this line contains target_link_libraries and ends with })
        if 'target_link_libraries' in line and line.strip().endswith('})'):
            # Look ahead to see if the next line contains ${PLATFORM_LIBS})
            if i + 1 < len(lines) and '${PLATFORM_LIBS})' in lines[i + 1]:
                # Fix by moving ${PLATFORM_LIBS} to the current line
                fixed_line = line.rstrip('})') + ' ${PLATFORM_LIBS})'
                fixed_lines.append(fixed_line)
                i += 2  # Skip the next line since we've merged it
                continue
        
        fixed_lines.append(line)
        i += 1
    
    content = '\n'.join(fixed_lines)
    
    # If content changed, write it back
    if content != original_content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Fixed syntax in {file_path}")
        return True
    
    return False

def main():
    """Main function to fix all CMakeLists.txt files"""
    
    # Find all CMakeLists.txt files in examples directory
    examples_dir = "examples"
    cmake_files = []
    
    for root, dirs, files in os.walk(examples_dir):
        for file in files:
            if file == "CMakeLists.txt":
                cmake_files.append(os.path.join(root, file))
    
    print(f"Found {len(cmake_files)} CMakeLists.txt files to check")
    
    fixed_count = 0
    
    for cmake_file in cmake_files:
        if fix_target_link_libraries_syntax(cmake_file):
            fixed_count += 1
    
    print(f"\nFixed syntax in {fixed_count} files")

if __name__ == "__main__":
    main() 