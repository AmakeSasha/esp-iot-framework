# SPDX-FileCopyrightText: 2026 AmakeSasha
# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Path: ./tools/mdns_discover.py
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

import sys
import json
import time
import argparse
import subprocess
from typing import List, Dict, Any

SERVICE_PREFIXES = ["_eif_http._tcp", "_eif_https._tcp"]

def ensure_dependency():
    try:
        import zeroconf
        return zeroconf
    except ImportError:
        print("Dependency 'zeroconf' not found.", file=sys.stderr)
        print("Attempting to install it automatically via pip...", file=sys.stderr)
        
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "zeroconf==0.132.2"])
            print("Successfully installed 'zeroconf'. Please re-run the script.", file=sys.stderr)
            sys.exit(0)
        except subprocess.CalledProcessError:
            print("ERROR: Failed to install 'zeroconf' automatically.", file=sys.stderr)
            print("Please install it manually:", file=sys.stderr)
            print(f"  {sys.executable} -m pip install zeroconf", file=sys.stderr)
            sys.exit(1)

zeroconf = ensure_dependency()
from zeroconf import Zeroconf, ServiceBrowser, ServiceListener, ServiceInfo

class MDNSListener(ServiceListener):
    def __init__(self, target_list: List[Dict[str, Any]]):
        self.found_devices = target_list

    def add_service(self, zc: Zeroconf, type_: str, name: str):
        info: ServiceInfo = zc.get_service_info(type_, name)
        if info:
            ips = []

            if info.addresses:
                ips.extend([".".join(map(str, addr)) for addr in info.addresses])

            if hasattr(info, 'addresses6') and info.addresses6:
                try:
                    ips.extend([addr.exploded for addr in info.addresses6])
                except AttributeError:
                    ips.extend([".".join(map(str, addr)) for addr in info.addresses6])

            device_entry = {
                "host": info.server.rstrip('.'),
                "ip": ips[0] if ips else "Unknown",
                "port": info.port,
                "ips": ips if ips else ["Unknown"]
            }

            if device_entry not in self.found_devices:
                self.found_devices.append(device_entry)

    def remove_service(self, zc: Zeroconf, type_: str, name: str):
        pass

    def update_service(self, zc: Zeroconf, type_: str, name: str):
        self.add_service(zc, type_, name)


def discover_devices(timeout: int = 3) -> Dict[str, List[Dict[str, Any]]]:
    zc = Zeroconf()
    
    http_devices: List[Dict[str, Any]] = []
    https_devices: List[Dict[str, Any]] = []
    
    listeners = {
        SERVICE_PREFIXES[0]: MDNSListener(http_devices),
        SERVICE_PREFIXES[1]: MDNSListener(https_devices)
    }

    browsers = []
    
    try:
        full_prefixes = [f"{p}.local." for p in SERVICE_PREFIXES]
        
        for full_prefix, listener in zip(full_prefixes, listeners.values()):
            browser = ServiceBrowser(zc, full_prefix, listener)
            browsers.append(browser)

        time.sleep(timeout)
        return {
            SERVICE_PREFIXES[0].replace("._tcp", ""): http_devices,
            SERVICE_PREFIXES[1].replace("._tcp", ""): https_devices
        }

    except Exception as e:
        return {"error": str(e)}
    finally:
        zc.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="mDNS Active Discovery for ESP IoT Framework", add_help=False)
    parser.add_argument("--timeout", type=int, default=3, help="Network scan timeout in seconds (default: 3)")
    args, unknown = parser.parse_known_args()

    devices = discover_devices(args.timeout)
    print(json.dumps(devices, indent=2, ensure_ascii=False))

    if "error" in devices:
        sys.exit(1)
        
    total_found = sum(len(lst) for lst in devices.values())
    if total_found == 0:
        sys.exit(1)
        
    sys.exit(0)