<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_server
  File: README.md
  Library: esp_iot_framework_server
  
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
    <h1>esp_iot_framework_server <small><font color="gray" style="font-weight: normal;">v0.1.0</font></small></h1>
    <h3>A framework node for deploying a standalone HTTP(S) server directly on the device to handle incoming requests</h3>
    <p>
        <code>esp32</code> &bull;
        <code>esp32s2</code> &bull;
        <code>esp32s3</code> &bull;
        <code>esp32c2</code> &bull;
        <code>esp32c3</code> &bull;
        <code>esp32c6</code>
    </p>
</div>

`esp_iot_framework_server` is a framework node for deploying a standalone HTTP(S) server directly on the device to handle incoming requests. Following the project's core philosophy of absolute autonomy, it turns the microcontroller into a self-sufficient server, enabling direct control via standard web browsers, <code>[curl](https://curl.se/)</code>, or similar CLI tools without requiring external hubs or applications. For an in-depth look, check out the <code>[documentation](../../docs/invalid_link.md#group__server__group.html)</code>.

<h2>Architecture and Under the Hood</h2>

The web server's lifecycle is tightly synchronized with network events: it spins up automatically as soon as the device acquires an IP address and gracefully stops, freeing up system resources, if the connection is lost.

The entire server infrastructure functions via a middleware layer that handles uniform request logging and restricts access to administrative endpoints using Basic Auth (when enabled in the project). The node supports both plain HTTP and secure HTTPS (TLS), utilizing certificates and configurations persisted in NVS.

<h2>REST API Features</h2>

The built-in API equips your end device with a scalable suite of management capabilities:

* **Wi-Fi Management** - Retrieve the list of saved Wi-Fi profiles, update or clear Wi-Fi profiles, and run background connectivity and health tests for a specific Wi-Fi profile.
* **System Metrics** - Access essential and expandable device telemetry, including heap statistics (free/minimum heap, largest free block), uptime, CPU frequency, core count, chip model and hardware revision, flash size, last reset reason, the device MAC address, and other system data.
* **OTA Firmware Management** - Read running software metadata (such as project name, version, compile date, and ESP-IDF version), upload new binaries, and handle update confirmations or automatic rollbacks.
* **Security Control** - Modify the administrator password on the fly and trigger on-device regeneration of cryptographic TLS keys and certificates.
* **Remote log reading** - View device logs directly in the browser without a UART connection. `CORE` captures all system and business logs into an internal ring buffer; the `SERVER` streams them live to the `Logs` page in the Web Admin GUI.

For an in-depth look, check out the <code>[API documentation](../../docs/invalid_link.md#group__server__rest__api.html)</code>.

<h2>Web Admin GUI</h2>

When the graphical interface is included in the build, its compressed static assets (HTML, CSS, JS) are embedded directly into the firmware binary. The interface is highly optimized for lightweight performance and maintains backward compatibility with legacy browsers (down to IE8 and below), except for the OTA update functionality, which requires browser support for JS file uploads.

<h2>Implementing Custom Device Logic</h2>

Transforming this foundational node into an end product is achieved by registering custom URI handlers for peripherals, sensors, or relays. By placing custom endpoints within the administrative URI space (using the `/_/` prefix), the node automatically applies the administrator's Basic Auth. This allows you to focus on your product's core logic without getting bogged down by low-level web server boilerplate.

<h1>Example of use</h1>

More examples can be found in the [`examples`](../../examples) folder with the suffix `_server`.

```C
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_iot_framework_core.h>
#include <esp_iot_framework_server.h>

esp_err_t hello_world(httpd_req_t *req) {
    const char *resp = "Hello World, from esp_iot_framework!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t my_uris[] = {
    { .uri = "/hello", .method = HTTP_GET, .handler = hello_world }
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_server_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 1));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

<h1>Usage for creating end devices</h1>

To start using this framework, you must apply the appropriate configuration files (from both the `CORE` and `SERVER` components) and connect it to your project. Configure your project's main `CMakeLists.txt` like this (replace `<TEXT>` with the desired values, without the `<>`s themselves):
```CMake
cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS 
    "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_core/sdkconfig.defaults"
    "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_server/sdkconfig.server"
)
# If you are using your own FILE, add this line:
# list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<NAME_PROJECT>)
```