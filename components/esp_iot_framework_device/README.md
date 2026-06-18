<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp_iot_framework
  Folder: ./components/esp_iot_framework_device
  File: README.md
  
  Copyright 2026 AmakeSasha
  
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  
    http://www.apache.org/licenses/LICENSE-2.0
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->

<div align="center">
    <h1>esp_iot_framework_device <small><font color="gray" style="font-weight: normal;">v0.2.1</font></small></h1>
    <h3>A framework for creating reliable IoT devices based on the Espressif Systems chips</h3>
    <p>
        <code>esp32</code> &bull; 
        <code>esp32s2</code> &bull; 
        <code>esp32s3</code> &bull; 
        <code>esp32c2</code> &bull; 
        <code>esp32c3</code> &bull; 
        <code>esp32c6</code>
    </p>
</div>

`esp_iot_framework_device` is a foundational network node based on Espressif chips, designed for building fully manageable end IoT products. Operating on top of `esp_iot_framework_core`, it provides an out-of-the-box HTTP/HTTPS web server with a comprehensive REST API for remote administration alongside an optional Web Admin GUI. For an in-depth look, check out the [documentation](../../docs/invalid_link#group__device__group.html).

<h2>Architecture and Under the Hood</h2>

The web server's lifecycle is tightly synchronized with network events: it spins up automatically as soon as the device acquires an IP address and gracefully stops, freeing up system resources, if the connection is lost.

The entire server infrastructure functions via a middleware layer that handles uniform request logging and restricts access to administrative endpoints using Basic Auth (when enabled in the project). The node supports both plain HTTP and secure HTTPS (TLS), utilizing certificates and configurations persisted in NVS.

<h2>REST API Features</h2>

The built-in API equips your end device with a scalable suite of management capabilities:

* **Wi-Fi Management** — Retrieve the list of saved Wi-Fi profiles, update or clear Wi-Fi profiles, and run background connectivity and health tests for a specific Wi-Fi profile.
* **System Metrics** — Access essential and expandable device telemetry, including heap statistics (free/minimum heap, largest free block), uptime, CPU frequency, core count, chip model and hardware revision, flash size, last reset reason, the device MAC address, and other system data.
* **OTA Firmware Management** — Read running software metadata (such as project name, version, compile date, and ESP-IDF version), upload new binaries, and handle update confirmations or automatic rollbacks.
* **Security Control** — Modify the administrator password on the fly and trigger on-device regeneration of cryptographic TLS keys and certificates.

For an in-depth look, check out the [API documentation](../../docs/invalid_link#group__device__rest__api.html).

<h2>Web Admin GUI</h2>

When the graphical interface is included in the build, its compressed static assets (HTML, CSS, JS) are embedded directly into the firmware binary. The interface is highly optimized for lightweight performance and maintains backward compatibility with legacy browsers (down to IE8 and below), except for the OTA update functionality, which requires browser support for JS file uploads.

<h2>Implementing Custom Device Logic</h2>

Transforming this foundational node into an end product is achieved by registering custom URI handlers for peripherals, sensors, or relays. By placing custom endpoints within the administrative URI space (using the `/_/` prefix), the node automatically applies the administrator's Basic Auth. This allows you to focus on your product's core logic without getting bogged down by low-level web server boilerplate.

<h1>Usage for creating end devices</h1>

To start using this framework, you must apply the settings from `sdkconfig.defaults` and connect it to your project. Configure your project's main `CMakeLists.txt` like this (replace `<TEXT>` with the desired values, without the `<>`s themselves):
```CMake
cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_device/sdkconfig.defaults")
# If you are using your own FILE, add this line:
# list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<NAME_PROJECT>)
```