# SPDX-License-Identifier: Apache-2.0
# Project: esp_iot_framework
# Folder: ./tools
# File: gen_docs.py
# 
# Copyright 2026 AmakeSasha
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os, subprocess, shutil, re

def get_dir_size(path):
    total = 0
    if not os.path.exists(path): return 0
    for root, _, files in os.walk(path):
        for f in files:
            total += os.path.getsize(os.path.join(root, f))
    return total

def generate_md_page(docs_dir, src_path, title, name):
    dst_path = os.path.normpath(os.path.join(docs_dir, "html", f"{name}_page.md"))

    if os.path.exists(src_path):
        with open(src_path, "r", encoding="utf-8") as src:
            content = src.read()

        with open(dst_path, "w", encoding="utf-8") as dst:
            dst.write(f"# {title}\n\n")
            dst.write("```text\n")
            dst.write(content)
            dst.write("\n```\n")

        return dst_path
    return None

def generate_readme(docs_dir, src_path, filename):
    dst_path = os.path.normpath(os.path.join(docs_dir, "html", filename))

    replacement_data = [
        {
            "original": r"\(./components/esp_iot_framework_core/README.md\)",
            "converted": "(group__core__root.html)"
        },
        {
            "original": r"\(./components/esp_iot_framework_device/README.md\)",
            "converted": "(group__device__root.html)"
        },
        {
            "original": r"\(./examples/LICENSE_CC0_1_0\)",
            "converted": "(md_docs_html_license_cc0_page.html)"
        },
        {
            "original": r"\(./examples\)",
            "converted": "(index.html)"
        },
        {
            "original": r"\(./LICENSE\)",
            "converted": "(md_docs_html_license_page.html)"
        },
        {
            "original": r"\(./NOTICE\)",
            "converted": "(md_docs_html_notice_page.html)"
        },
    ]

    if os.path.exists(src_path):
        with open(src_path, "r", encoding="utf-8") as src:
            content = src.read()

            content = re.sub(r"\(\.\./\.\./docs/invalid_link#(group__.*?\.html)\)", r"(\1)", content)

            for item in replacement_data:
                content = re.sub(item["original"], item["converted"], content)

        with open(dst_path, "w", encoding="utf-8") as dst:
            dst.write(content)

        return dst_path
    return None

def main():
    print(">>> Init script...")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.normpath(os.path.join(script_dir, ".."))
    docs_dir = os.path.normpath(os.path.join(project_root, "docs"))
    html_dir = os.path.normpath(os.path.join(docs_dir, "html"))

    list_files = [
        {
            "src": "LICENSE",
            "title": "Apache 2.0 License",
            "name": "license",
            "type": "file",
            "dst_path": ""
        },
        {
            "src": "examples/LICENSE_CC0_1_0",
            "title": "CC0 1.0 License",
            "name": "license_cc0",
            "type": "file",
            "dst_path": ""
        },
        {
            "src": "NOTICE",
            "title": "NOTICE for Apache 2.0",
            "name": "notice",
            "type": "file",
            "dst_path": ""
        },
        {
            "src": "README.md",
            "name": "README.md",
            "type": "readme",
            "dst_path": ""
        },
        {
            "src": "components/esp_iot_framework_core/README.md",
            "name": "README_CORE.md",
            "type": "readme",
            "dst_path": ""
        },
        {
            "src": "components/esp_iot_framework_device/README.md",
            "name": "README_DEVICE.md",
            "type": "readme",
            "dst_path": ""
        }
    ]

    os.chdir(docs_dir)
    if os.path.exists(html_dir):
        shutil.rmtree(html_dir)
    os.makedirs(html_dir, exist_ok=True)

    print(">>> Formating files...")
    for item in list_files:
        src_path = os.path.normpath(os.path.join(project_root, item["src"]))
        
        if item["type"] == "file":
            item["dst_path"] = generate_md_page(
                docs_dir, src_path, item["title"], item["name"]
            )
        elif item["type"] == "readme":
            item["dst_path"] = generate_readme(docs_dir, src_path, item["name"])

    print(">>> Launching Doxygen...")
    process = subprocess.Popen(
        "doxygen doxygen/Doxyfile", 
        shell=True, 
        stderr=subprocess.PIPE, 
        text=True, 
        encoding="utf-8"
    )
    _, stderr_output = process.communicate()
    
    if stderr_output:
        for line in stderr_output.splitlines():
            if "Unexpected html tag <font>" not in line and "Unexpected html tag </font>" not in line:
                print(line)

    print(">>> Removing formatted files...")
    for item in list_files:
        os.remove(item["dst_path"])

    size = get_dir_size("html")

    print(f"\n>>> Done, size website: {size/1024:.1f} KB")

if __name__ == "__main__":
    main()
