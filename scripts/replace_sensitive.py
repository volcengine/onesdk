import os
import re
import sys

# 定义要处理的目录
directories = ["examples", "tests"]

# 定义需要处理的宏名列表
macros_to_replace = {
    # "SAMPLE_HTTP_HOST",
    "SAMPLE_INSTANCE_ID",
    "SAMPLE_MQTT_HOST",
    "SAMPLE_DEVICE_NAME",
    "SAMPLE_DEVICE_SECRET",
    "SAMPLE_PRODUCT_KEY",
    "SAMPLE_PRODUCT_SECRET",
    "CONFIG_WIFI_SSID",
    "CONFIG_WIFI_PASSWORD",
}

# 构建正则表达式
# 匹配: 可选空格, 可选 "// " 注释, #define, 空格, (宏名之一), 空格, "任意字符串"
# 捕获组:
# 1: 前导部分 (可选空格, 可选注释, #define, 空格, 宏名, 空格)
# 2: 宏名 (用于检查是否在列表中)
# 3: 引号内的原始值 (将被替换)
# 使用 re.escape 对宏名进行转义，以防宏名包含特殊正则字符
macro_pattern = "|".join(re.escape(m) for m in macros_to_replace)
# regex = re.compile(r'^(\s*(?://\s*)?#define\s+(' + macro_pattern + r')\s+)"(.*)"')
# 改进正则，确保只匹配宏定义行，并正确处理注释和空格
regex = re.compile(r'^(\s*(//\s*)?#define\s+(' + macro_pattern + r')\s+)"(.*)"')


def process_file(filepath):
    """处理单个文件，替换敏感宏定义"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError:
        try:
            # 如果 UTF-8 失败，尝试使用系统默认编码或其他常见编码
            with open(filepath, 'r', encoding=sys.getdefaultencoding()) as f:
                lines = f.readlines()
        except Exception as e:
            print(f"  错误: 无法读取文件 {filepath}: {e}")
            return False
    except Exception as e:
        print(f"  错误: 无法打开文件 {filepath}: {e}")
        return False


    modified_lines = []
    file_changed = False
    for line in lines:
        match = regex.match(line)
        # 检查捕获组3是否存在且匹配的宏名在我们的列表中
        if match and match.group(3) in macros_to_replace:
            # 构建替换后的行：捕获组1 + "***"
            # 捕获组1 包含了从行首到宏名后的所有内容（包括引号前）
            new_line = f'{match.group(1)}"***"\n'
            modified_lines.append(new_line)
            if line != new_line: # 仅当实际发生更改时标记
                file_changed = True
        else:
            modified_lines.append(line)

    if file_changed:
        try:
            # 创建备份文件
            backup_filepath = filepath + ".bak"
            os.rename(filepath, backup_filepath)
            print(f"  已备份原文件到: {backup_filepath}")

            # 写回修改后的内容
            with open(filepath, 'w', encoding='utf-8') as f:
                f.writelines(modified_lines)
            print(f"  已修改文件: {filepath}")
            return True
        except Exception as e:
            print(f"  错误: 无法写入文件 {filepath} 或创建备份: {e}")
            # 尝试恢复备份
            if os.path.exists(backup_filepath):
                try:
                    os.rename(backup_filepath, filepath)
                    print(f"  已从备份恢复: {filepath}")
                except Exception as restore_e:
                     print(f"  严重错误: 无法恢复备份文件 {backup_filepath}: {restore_e}")
            return False
    else:
        # print(f"  文件无需修改: {filepath}")
        return False

# --- 主程序 ---
print("开始替换敏感数据...")
total_modified = 0

for directory in directories:
    if not os.path.isdir(directory):
        print(f"警告: 目录 '{directory}' 不存在，已跳过。")
        continue

    print(f"正在处理目录: {directory}")
    for root, _, files in os.walk(directory):
        for filename in files:
            if filename.endswith((".c", ".h")):
                filepath = os.path.join(root, filename)
                if process_file(filepath):
                    total_modified += 1

print(f"\n敏感数据替换完成。共修改了 {total_modified} 个文件。")
print("已创建备份文件（扩展名为 .bak）。请检查修改结果。")
print("如果确认无误，可以手动删除 .bak 文件，或运行以下命令批量删除:")
print(f"find {' '.join(directories)} -type f -name '*.bak' -delete")