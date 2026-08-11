<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_core
  File: README.md
  Library: esp_iot_framework_core
  
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
    <h1>esp_iot_framework_core <small><font color="gray" style="font-weight: normal;">v0.2.1</font></small></h1>
    <h3>A meta-framework for building reliable, node-based IoT frameworks on Espressif Systems devices</h3>
    <p>
        <code>esp32</code> &bull;
        <code>esp32s2</code> &bull;
        <code>esp32s3</code> &bull;
        <code>esp32c2</code> &bull;
        <code>esp32c3</code> &bull;
        <code>esp32c6</code>
    </p>
</div>

`esp_iot_framework_core` is the core component of the framework that handles low-level device infrastructure boilerplate. It ensures the device connects to the network automatically, stores data securely, and runs stably without crashing. It is designed for building network nodes rather than standalone end devices (though the latter is technically possible but not recommended).

<h2>Key Features</h2>

* Initializes Wi-Fi in STA mode, handles network events, and cycles through saved profiles to restore connection during failures.
* Provides an abstraction layer over NVS to reliably store Wi-Fi configurations, TLS certificates, and Basic Auth credentials.
* Generates ECC keys (`secp256r1` curve) and X.509 certificates with SAN extensions directly on the device, automatically saving them to NVS.
* Fully manages the mDNS subsystem lifecycle from initialization and HTTP/HTTPS service registration to resource cleanup.
* Includes a set of handy macros that simplify logging, debugging, and error handling.
* Periodically checks heap fragmentation and triggers a preventive auto-reboot if memory degradation hits a critical threshold.
* Log capture layer that hooks into the ESP-IDF logging system, buffers all runtime logs into a ring buffer, and exposes them for any Node to fetch and transmit to the outside world.

For an in-depth look, check out the documentation for each section:
* [Public Core](../../docs/invalid_link.md#group__core__group.html)
* [Core Extensions](../../docs/invalid_link.md#group__core__ext__group.html)
* [Core Macros](../../docs/invalid_link.md#group__core__macros__group.html)

<h1>Usage for creating nodes</h1>

Configure your project's main `CMakeLists.txt` like this (replace `<TEXT>` with the desired values, without the `<>`s themselves):

```CMake
set(COMPONENT_NAME <COMPONENT_NAME>)

idf_component_register(
    SRCS <SOURCE_FILES>
    INCLUDE_DIRS <PUBLIC_INCLUDE_DIRS>
    PRIV_INCLUDE_DIRS <PRIVATE_INCLUDE_DIRS>
    PRIV_REQUIRES <DEPENDENCIES> esp_iot_framework_core
    EMBED_FILES <FILES_TO_EMBED>
)

# Required for logging macros
set(comp_target "__idf_${COMPONENT_NAME}")
target_compile_options(${comp_target} PRIVATE "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}=.")
```

In the destination device, you will need to specify the path to the component. You can do this using one of the following methods:

* <b>(Recommended)</b> Add the following code to the root `CMakeLists.txt`:
  ```CMake
  cmake_minimum_required(VERSION 3.16)
 
  # Necessary for correct operation
  list(APPEND SDKCONFIG_DEFAULTS "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_device/sdkconfig.defaults")
  list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")
 
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
  project(hello_world_test)
  ```

* Add the following code to `idf_component.yml` or integrate it into an existing one:
  ```yaml
  dependencies:
    esp_iot_framework_core:
      path: "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_core"
  ```

* Alternatively, add the framework as a Git submodule inside your project's `components` directory. ESP-IDF will detect it automatically, and the code will work without any extra configuration.