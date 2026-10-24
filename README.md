<!--
  SPDX-FileCopyrightText: 2026 AmakeSasha
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Path: ./README.md
  
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
    <h1>esp_iot_framework <sup>v0.3.0</sup></h1>
    <h3>A framework for building reliable IoT devices on Espressif SoCs using ESP-IDF</h3>
    <p>
        <code>esp32</code> &bull;
        <code>esp32s2</code> &bull;
        <code>esp32s3</code> &bull;
        <code>esp32c2</code> &bull;
        <code>esp32c3</code> &bull;
        <code>esp32c6</code>
    </p>
    <p>
        ESP-IDF <code>v4.4.8</code> - <code>v5.5</code>
    </p>
</div>

`esp_iot_framework` significantly reduces the need for embedded developers to write repetitive infrastructure boilerplate for Espressif chips, providing a robust foundation for transforming a microcontroller into an autonomous, secure, and resilient local network server node.

Out of the box, it provides an event-driven Wi-Fi management loop that automatically cycles through stored profiles to restore connectivity, alongside a native mDNS responder that makes the device discoverable through a stable hostname rather than a changing IP address.

Security is handled by an on-device cryptographic pipeline that generates ECC key pairs and self-signs X.509 certificates with Subject Alternative Name (SAN) extensions to enable HTTPS communication. Cryptographic material and configuration parameters are persisted through an abstraction layer over non-volatile storage.

Running directly on the SoC, the framework exposes a REST API and a lightweight web administration interface, allowing developers to configure Wi-Fi profiles, inspect detailed hardware telemetry, change administrative passwords dynamically, and perform OTA firmware updates with verification and rollback support.

Designed for operational resilience, the framework captures native ESP-IDF log output in an internal ring buffer and streams it directly to the browser without requiring a physical UART connection. A proactive memory monitor tracks heap fragmentation and can trigger controlled preventive reboots when configured thresholds are exceeded.

This infrastructure lets developers focus on their application's core business logic. The framework's built-in administrative endpoints are protected by middleware-enforced Basic Auth; endpoints you register via `server_uri_handlers` are not.

---

# Quick start

## Configuration

Add the framework to your project by configuring the root `CMakeLists.txt` file (replace `<PATH_TO_FRAMEWORK>` and `<PROJECT_NAME>` with your actual values, omitting the `<>`):

```CMake
cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS 
    "<PATH_TO_FRAMEWORK>/components/esp_iot_framework/sdkconfig.defaults"
)
list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<PROJECT_NAME>)
```

> 📌 NOTE
>
> If the Kconfig option `CONFIG_EIF_ENABLE_WEB_ADMIN_GUI` is enabled (*default*: `y`), building requires the `gzip` utility available in `PATH` to compress the embedded web assets. It is preinstalled on Linux/macOS; on Windows use Git Bash, WSL, or MSYS2.

## Usage Examples

Other examples can be found in the [`examples`](./examples) folder.

```C
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_iot_framework.h>

esp_err_t hello_world(httpd_req_t *req) {
    const char *resp = "Hello World, from esp_iot_framework!";
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static const httpd_uri_t my_uris[] = {
    { .uri = "/", .method = HTTP_GET, .handler = hello_world }
};

void app_main(void) {
    eif_init_config_t cfg = eif_init_config_default();

    cfg.server_uri_handlers = my_uris;
    cfg.server_uri_handlers_count = sizeof(my_uris) / sizeof(my_uris[0]);

    ESP_ERROR_CHECK(eif_initialize(&cfg));

    vTaskDelete(NULL);
}
```

On first boot, the device connects to the built-in recovery Wi-Fi profile: SSID `ESP32_SETUP`, password `12345678`. Create a hotspot with these credentials (for example, on your phone), open the admin panel at `http(s)://[hostname]-[3_bytes_of_MAC].local`, log in as `admin` with an empty password, and configure your own network in the **`NETWORK`** tab and a new password in the **`SYSTEM`** tab.

## Configuration structure eif_init_config_t

You can modify the following fields before calling `eif_initialize`:
* `wifi_ps_type_t wifi_power_mode` (*default*: `WIFI_PS_NONE`) The Wi-Fi power save mode. Allows you to configure the Wi-Fi driver's sleep behavior to save power or disable it completely for maximum stability.
* `const char *mdns_hostname` (*default*: `"eif-device"`) The network hostname for the device in the local network. The framework automatically appends a suffix based on the device's MAC address to prevent name collisions. The final local address will be `[hostname]-[3_bytes_of_MAC].local`.
* `const char *mdns_instance_name` (*default*: `"EIF device"`) A human-readable device name that will be displayed in the network environment (e.g., in mDNS discovery applications).
* `const mdns_txt_item_t *mdns_txt_records` (*default*: `NULL`) An array of custom mDNS TXT records (`key`/`value` pairs) used to advertise additional device metadata over the network.
* `size_t mdns_txt_records_count` (*default*: `0`) The number of custom TXT records passed in the `mdns_txt_records` array. The maximum allowed count is `32`.
* `void (*sys_handler_reboot)(void)` (*default*: `NULL`) A function pointer callback. The framework will invoke this right before any system reboot (e.g., after an OTA firmware update), giving your application a chance to safely shut down hardware peripherals or save data to Flash.
* `const httpd_uri_t *server_uri_handlers` (*default*: `NULL`) An array of your custom HTTP endpoints (URIs). The framework automatically registers these on the web server alongside its internal system administration endpoints. May be `NULL` when `server_uri_handlers_count` is `0`, meaning no custom endpoints are registered.
* `size_t server_uri_handlers_count` (*default*: `0`) The number of custom endpoints passed in the `server_uri_handlers` array. When set to `0`, the `server_uri_handlers` pointer may be `NULL`.
* `httpd_ssl_config_t` / `httpd_config_t server_cfg` (*default*: Pre-configured by `eif_init_config_default()`) The underlying ESP-IDF web server configuration structure. Its actual type (`httpd_ssl_config_t` or `httpd_config_t`) is determined automatically at compile time based on the `CONFIG_EIF_ENABLE_TLS` setting.
---

# Documentation

* [REST API](./docs/REST_API.md)
* [Kconfig options](./docs/KCONFIG.md)

---

# Mirrors

The repository is synchronized between independent platforms to ensure availability:
* [**GitVerse**](https://gitverse.ru/amakesasha/esp-iot-framework) (main)
* [**GitHub**](https://github.com/AmakeSasha/esp-iot-framework) (mirror)
* [**GitCode**](https://gitcode.com/AmakeSashaDev/esp-iot-framework) (mirror)
* [**SourceCraft**](https://sourcecraft.dev/amakesasha/esp-iot-framework) (mirror)

---

# Development status

> ℹ️ Continuous Integration (CI) is not configured yet. The project is tested manually on ESP-IDF `v4.4.8` and `v5.5`, with both `CONFIG_EIF_ENABLE_TLS` and the Web Admin GUI toggled on and off.

---

# Third-Party Components

This framework bundles the following third-party components inside the `components/` directory:

* [`jsmn`](https://github.com/zserge/jsmn) - JSMN: minimalistic JSON parser in C (`MIT license`)
* [`json_generator`](https://github.com/shahpiyushv/json_generator) - A simple JSON (JavaScript Object Notation) generator with flushing capability (`Apache 2.0 license`)

---

# License

The source code of the framework and its documentation are licensed under the [`Apache-2.0 License`](./LICENSE.txt).

All code examples (located in the `./examples` directory) are dedicated to the public domain under the [`CC0-1.0`](./examples/LICENSE.txt).

Please also refer to the [`NOTICE.txt`](./NOTICE.txt) file for third-party attributions.