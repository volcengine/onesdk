#!/bin/bash
set -euo pipefail

cd build

# 合并所有 cpputest_ 开头的 JUnit XML 报告
INPUT_FILES=(cpputest_*.xml)
OUTPUT_FILE="report.xml"

# 检查输入文件是否存在
if [ ${#INPUT_FILES[@]} -eq 0 ]; then
    echo "Error: No cpputest_*.xml files found"
    exit 1
fi

# 检查 junitparser 是否安装
if ! python3 -c "import junitparser" >/dev/null 2>&1; then
    echo "Installing junitparser..."
    pip3 install junitparser
fi

# 执行合并操作
echo "Merging ${#INPUT_FILES[@]} test reports..."
python3 -m junitparser merge "${INPUT_FILES[@]}" "${OUTPUT_FILE}"

echo "Merged report saved to: ${OUTPUT_FILE}"

cd ..