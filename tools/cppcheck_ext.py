# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Folder: ./tools
# File: cppcheck_ext.py
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

import subprocess, sys, os

def callback():
    print(">>> Running `cppcheck` (MISRA C)...")


    cwd = os.getcwd()


    config_dir = cwd
    while config_dir and not os.path.exists(os.path.join(config_dir, "esp_iot_framework.cppcheck")):
        parent = os.path.dirname(config_dir)
        if parent == config_dir:
            break
        config_dir = parent

    os.makedirs(f"{config_dir}/esp_iot_framework_cppcheck_build_dir", exist_ok=True)


    command = [
        "cppcheck",
        f"--project={config_dir}/esp_iot_framework.cppcheck",
        # "--addon=misra",
        "-I", "build/config"
    ]


    try:
        res = subprocess.run(command, check=True, text=True, cwd=config_dir, timeout=120)
        print(">>> `cppcheck` analysis completed successfully")
        sys.exit(res.returncode)
    except subprocess.CalledProcessError as e:
        print(f">>> Error: `cppcheck` failed with exit code: {e.returncode}")
        print(f">>> Stderr: {e.stderr}")
        sys.exit(e.returncode)
    except FileNotFoundError:
        print(">>> Error: 'cppcheck' not found. Install it via your package manager")
        print(">>>   Ubuntu/Debian: sudo apt install cppcheck")
        print(">>>   macOS: brew install cppcheck")
        sys.exit(1)
    except OSError as e:
        print(f">>> Error: Failed to run `cppcheck`: {e.strerror}")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(">>> Error: `cppcheck` timed out (120s). Project may be too large.")
        sys.exit(1)
    except PermissionError:
        print(">>> Error: No permission to execute `cppcheck` or access project files")
        sys.exit(1)
    except MemoryError:
        print(">>> Error: Not enough memory to run `cppcheck`")
        sys.exit(1)

if __name__ == "__main__":
    callback()