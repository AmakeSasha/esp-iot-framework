<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_client
  File: README.md
  Library: esp_iot_framework_client
  
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
    <h1>esp_iot_framework_client <small><font color="gray" style="font-weight: normal;">v0.0.0</font></small></h1>
    <h3>A framework node for enabling the device to initiate outbound HTTP(S) connections and send requests to other devices</h3>
    <p>
        <code>esp32</code> &bull;
        <code>esp32s2</code> &bull;
        <code>esp32s3</code> &bull;
        <code>esp32c2</code> &bull;
        <code>esp32c3</code> &bull;
        <code>esp32c6</code>
    </p>
</div>

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

**THIS MODULE IS UNDER DEVELOPMENT AND IS NOT YET AVAILABLE FOR USE**

<h1>Usage for creating end devices</h1>

To start using this framework, you must apply the appropriate configuration files (from both the `CORE` and `CLIENT` components) and connect it to your project. Configure your project's main `CMakeLists.txt` like this (replace `<TEXT>` with the desired values, without the `<>`s themselves):
```CMake
cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS 
    "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_core/sdkconfig.defaults"
    "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_client/sdkconfig.client"
)
# If you are using your own FILE, add this line:
# list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<NAME_PROJECT>)
```