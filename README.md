<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: .
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
    <h1>esp_iot_framework <small><font color="gray" style="font-weight: normal;">v0.2.1</font></small></h1>
    <h3>A framework for building scalable IoT ecosystems, custom nodes, and smart end-devices</h3>
    <p>
        `esp32` •
        `esp32s2` •
        `esp32s3` •
        `esp32c2` •
        `esp32c3` •
        `esp32c6`
    </p>
</div>

# Core Architecture Concept

The framework is built around three fundamental concepts, decoupling the infrastructure from your business logic:

* <b>CORE ([`esp_iot_framework_core`](./components/esp_iot_framework_core/README.md))</b> - The ecosystem engine. It handles connectivity, security, and device resilience, abstracting away low-level services like Wi-Fi, NVS, and TLS. Anything built on top of *CORE* inherits these critical properties for free.

---

* **Nodes** – Architectural components built on top of *CORE*. A Node encapsulates a specific environment or interaction model for the device-such as hosting a server, initiating client requests, or executing isolated scripts. It offloads all low-level routine and routing, providing a clean interface for business logic development.

  Examples of nodes:

  * <b>SERVER ([`esp_iot_framework_server`](./components/esp_iot_framework_server/README.md))</b> - A framework node for deploying a standalone HTTP(S) server directly on the device to handle incoming requests.

  * <b>CLIENT ([`esp_iot_framework_client`](./components/esp_iot_framework_client/README.md))</b> - A framework node for enabling the device to initiate outbound HTTP(S) connections and send requests to other devices.

---

* **End Devices** – The final products built by combining *CORE* with one or multiple *Nodes* and your specific business logic. Smart bulbs, relays, or complex controllers - any target scenario boils down to implementing unique product logic on top of the selected node stack.

  You can see examples of end devices in the [`examples`](./examples) folder.

---

# Example of use

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
    ESP_ERROR_CHECK(eif_server_set_uri_handlers(my_uris, 1));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

# Storage, documentation and mirrors

To protect against network restrictions, blockages, and infrastructure failures, the project source code and technical documentation are synchronized between independent platforms.

| Platform               | Links                                                                                                                                                                      |
|:---------------------- |:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **SourceCraft** (main) | [🔗 **Source code**](https://sourcecraft.dev/amakesasha/esp-iot-framework) <br> [📄 **Documentation**](https://amakesasha.sourcecraft.site/esp-iot-framework/)             |
| **GitHub** (mirror)    | [🔗 **Source code**](https://github.com/AmakeSasha/esp-iot-framework) <br>      [📄 **Documentation**](https://amakesasha.github.io/esp-iot-framework/html/index.html)     |
| **GitCore** (mirror)   | [🔗 **Source code**](https://gitcode.com/AmakeSashaDev/esp-iot-framework) <br>      **Without documentation**                                                              |
| **GitVerse** (mirror)  | [🔗 **Source code**](https://gitverse.ru/amakesasha/esp-iot-framework) <br>     [📄 **Documentation**](https://amakesasha.gitverse.site/esp-iot-framework/html/index.html) |

# Third-Party Components

This framework bundles the following third-party components inside the `components/` directory:

* [`json2`](https://github.com/douglascrockford/JSON-js/) - JSON in JavaScript (`Public Domain` or `CC0-1.0`, at your discretion)
* [`jsmn`](https://github.com/zserge/jsmn) - JSMN: minimalistic JSON parser in C (`MIT license`)
* [`json_generator`](https://github.com/shahpiyushv/json_generator) - A simple JSON (JavasScript Object Notation) generator with flushing capability (`Apache 2.0 license`)

# License

This project is licensed under the [`Apache 2.0 License`](./LICENSE).

Please also refer to the [`NOTICE`](./NOTICE) file.