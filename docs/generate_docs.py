# SPDX-License-Identifier: Apache-2.0
# Project: esp_iot_framework
# Folder: docs
# File: generate_docs.py
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
from css_html_js_minify import html_minify, css_minify
from jsmin import jsmin

def get_dir_size(path):
    total = 0
    if not os.path.exists(path): return 0
    for root, _, files in os.walk(path):
        for f in files:
            total += os.path.getsize(os.path.join(root, f))
    return total

def main():
    if os.path.exists("html"):
        shutil.rmtree("html")
    
    print(">>> Launching Doxygen...")
    subprocess.run("doxygen Doxyfile", shell=True)

    target = os.path.join("html", "examples")
    os.makedirs(target, exist_ok=True)
    src = os.path.join("..", "examples", "LICENSE_CC0_1_0")
    if os.path.exists(src):
        shutil.copy(src, target)

    size = get_dir_size("html")


    print(f"\n--- Done ---")
    print(f"Size: {size/1024:.1f} KB")

if __name__ == "__main__":
    main()
