import os

LICENSE_HEADER = """// Copyright (2025) Beijing Volcano Engine Technology Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

"""

def add_license_to_files(directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.h', '.c', '.cpp', '.cc', '.hpp')):
                file_path = os.path.join(root, file)
                with open(file_path, 'r+', encoding='utf-8') as f:
                    content = f.read()
                    # 跳过已包含license的文件
                    if "Copyright (2025) Beijing Volcano Engine" not in content:
                        f.seek(0, 0)
                        f.write(LICENSE_HEADER + content)

if __name__ == "__main__":
    target_dir = input("请输入要添加License的目录路径: ")
    add_license_to_files(target_dir)
    print(f"License头已添加到{target_dir}目录下的所有.h/.c/.cpp/.cc/.hpp文件")
