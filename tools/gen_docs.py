# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
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

REPLACEMENT_DATA = [
    {
        "original": r"\(./components/esp_iot_framework_core/README.md\)",
        "converted": "(group__core__root.html)"
    },
    {
        "original": r"\(./components/esp_iot_framework_server/README.md\)",
        "converted": "(group__server__root.html)"
    },
    {
        "original": r"\(./components/esp_iot_framework_client/README.md\)",
        "converted": "(group__client__root.html)"
    },
    {
        "original": r"\(./examples\)",
        "converted": "(index.html)"
    },
    {
        "original": r"\(../../examples\)",
        "converted": "(index.html)"
    },
    {
        "original": r"\(./LICENSE\)",
        "converted": "(md_docs_html_license_page.html)"
    },
    {
        "original": r"\(./NOTICE\)",
        "converted": "(md_docs_html_notice_page.html)"
    }
]

LIST_FILES = [
    {
        "src": "LICENSE",
        "title": "Apache 2.0 License",
        "name": "license",
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
        "src": "components/esp_iot_framework_core/KCONFIG.md",
        "name": "KCONFIG_CORE.md",
        "type": "readme",
        "dst_path": ""
    },
    {
        "src": "components/esp_iot_framework_server/README.md",
        "name": "README_SERVER.md",
        "type": "readme",
        "dst_path": ""
    },
    {
        "src": "components/esp_iot_framework_server/KCONFIG.md",
        "name": "KCONFIG_SERVER.md",
        "type": "readme",
        "dst_path": ""
    },
    {
        "src": "components/esp_iot_framework_server/REST_API.md",
        "name": "REST_API_SERVER.md",
        "type": "readme",
        "dst_path": ""
    },
    {
        "src": "components/esp_iot_framework_client/README.md",
        "name": "README_CLIENT.md",
        "type": "readme",
        "dst_path": ""
    },
]

# --------------------------------------------------------------------------------
#                                 FILE PREPARATION
# --------------------------------------------------------------------------------
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

    if os.path.exists(src_path):
        with open(src_path, "r", encoding="utf-8") as src:
            content = src.read()

            content = re.sub(
                r'href="[^"]*"\s+real_ref="([^"]*)"', 
                r'href="\1"', 
                content
            )

            content = re.sub(
                r'(?<=\()\.\./\.\./docs/invalid_link\.md#(group__.*?\.html[^)]*)(?=\))', 
                r'\1', 
                content
            )

            for item in REPLACEMENT_DATA:
                content = re.sub(item["original"], item["converted"], content)

        with open(dst_path, "w", encoding="utf-8") as dst:
            dst.write(content)

        return dst_path
    return None

def formatted_files(project_root, docs_dir, html_dir):
    os.chdir(docs_dir)
    if os.path.exists(html_dir):
        shutil.rmtree(html_dir)
    os.makedirs(html_dir, exist_ok=True)

    print(">>> Formating files...")
    for item in LIST_FILES:
        src_path = os.path.normpath(os.path.join(project_root, item["src"]))
       
        if item["type"] == "file":
            item["dst_path"] = generate_md_page(
                docs_dir, src_path, item["title"], item["name"]
            )
        elif item["type"] == "readme":
            item["dst_path"] = generate_readme(docs_dir, src_path, item["name"])
# --------------------------------------------------------------------------------
#                             END FILE PREPARATION
# --------------------------------------------------------------------------------



# --------------------------------------------------------------------------------
#                                  LAUNCH DOXYGEN
# --------------------------------------------------------------------------------
def launch_doxygen():
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
# --------------------------------------------------------------------------------
#                              END LAUNCH DOXYGEN
# --------------------------------------------------------------------------------



# --------------------------------------------------------------------------------
#                                  FINAL GENERATE
# --------------------------------------------------------------------------------
def remove_formatted_files():
    print(">>> Removing formatted files...")
    for item in LIST_FILES:
        print(f"Delete file {item['dst_path']}")
        os.remove(item["dst_path"])

        html_folder = os.path.dirname(item["dst_path"])
        orig_name = os.path.basename(item["src"])
        target_to_delete = os.path.normpath(os.path.join(html_folder, orig_name))
        
        if os.path.exists(target_to_delete):
            print(f"Delete file {target_to_delete}")
            os.remove(target_to_delete)

def website_simplification(html_dir):
    print(">>> Website simplification...")

    dir_file_pattern = re.compile(r"^dir_[a-fA-F0-9]{32}\.html$")

    pattern_comment_html = re.compile(r"<!--(?:[^-]|-(?!->))*-->\n|<!--(?:[^-]|-(?!->))*-->")
    pattern_comment_js = re.compile(r"/\*(?:[^*]|\*(?!/))*\*/(?![^\n\r]*?\\)\r?\n?")
    
    pattern_single = re.compile(r"//(?![^\r\n]*[^a-zA-Z0-9\s\"'\"()\[\]{}])[^\r\n]*\r?\n?")
    pattern_empty_lines = re.compile(r"(\r?\n)\s*\r?\n")
    pattern_indent = re.compile(r"^\s+", re.MULTILINE)

    for root, _, files in os.walk(html_dir):
        for file in files:
            if dir_file_pattern.match(file):
                path = os.path.join(root, file)
                os.remove(path)
                continue

            if file.endswith((".html", ".js", ".css")):
                path = os.path.join(root, file)
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()

                cleaned = pattern_comment_html.sub("", content)
                cleaned = pattern_comment_js.sub("", cleaned)

                cleaned = pattern_single.sub("", cleaned)
                cleaned = pattern_empty_lines.sub(r"\1", cleaned)
                cleaned = pattern_indent.sub("", cleaned)

                with open(path, "w", encoding="utf-8") as f:
                    f.write(cleaned)

def print_size_website(path):
    total = 0
    if not os.path.exists(path): return 0
    for root, _, files in os.walk(path):
        for f in files:
            total += os.path.getsize(os.path.join(root, f))

    print(f"\n>>> Done, size website: {total/1024:.1f} KB")
# --------------------------------------------------------------------------------
#                              END FINAL GENERATE
# --------------------------------------------------------------------------------



# --------------------------------------------------------------------------------
#                                       MAIN
# --------------------------------------------------------------------------------
def main():
    print(">>> Init script...")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.normpath(os.path.join(script_dir, ".."))
    docs_dir = os.path.normpath(os.path.join(project_root, "docs"))
    html_dir = os.path.normpath(os.path.join(docs_dir, "html"))

    formatted_files(project_root, docs_dir, html_dir)
    launch_doxygen()
    remove_formatted_files()
    website_simplification(html_dir)
    print_size_website(docs_dir)

if __name__ == "__main__":
    main()
# --------------------------------------------------------------------------------
#                                   END MAIN
# --------------------------------------------------------------------------------