<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_device
  File: KCONFIG.md
  Library: esp_iot_framework_device
  
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

# Kconfig esp_iot_framework_device

This section describes the `esp_iot_framework_device` configuration parameters. All settings are available in the `idf.py menuconfig` configuration utility within the `ESP IoT framework Device` menu.

---

`Web admin GUI` -> `EIF_ENABLE_WEB_ADMIN_GUI` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable Web-based Administration Panel`

Enables the integrated web interface for framework management. This affects **ONLY** the administrative panel, not your custom user UI.

- If `y`: All static web assets (HTML, CSS, JS) from the `web_src` directory are compressed and embedded directly into the firmware flash.
- If `n`: The entire Web GUI is stripped from the build. The device will expose only the raw JSON REST API for remote integration.

Disabling this feature significantly reduces Flash usage and optimizes the HTTP server's memory footprint by completely removing the file-serving routing logic.

---

`Web admin GUI` -> `EIF_ENABLE_WEB_FAVICON` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Embed Web Favicon (Logo)`

Includes the official framework branding icon in the firmware.

- If `y`: The PNG favicon image is compiled into Flash and automatically served on `/favicon.ico` requests.
- If `n`: Strips the asset from the firmware, saving extra Flash space and removing the specific URI route handler.

---

`Web admin GUI` -> `EIF_WEB_CACHE_MAX_AGE` - `int` (*range*: `0` - `31536000`, *default*: `360`)

Name in the `menuconfig`: `Static files cache max-age (seconds)`

Sets the `max-age` value (in seconds) for the HTTP `Cache-Control` header. During this period, the browser serves web pages strictly from its local cache without hitting the ESP32, minimizing CPU and heap usage.

- Production: Set a high value (e.g., `86400`) to avoid repetitive TLS handshakes and save MCU resources.
- Development: Set strictly to `0`. Otherwise, browser caching will hide your web source modifications until a hard refresh (`Ctrl` + `F5`) is forced.

<div style="background-color: #d1ecf1; border-left: 5px solid #17a2b8; padding: 12px; margin: 10px 0; color: #0c5460;">
    <strong>Note</strong><br>
    <code>ETag</code> validation occurs only <b>AFTER</b> this timer expires or if the user performs a manual refresh.
</div>

---

`EIF_ENABLE_BASIC_AUTH` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable Basic Auth for the core URL (/_/*)`

Enables mandatory HTTP Basic Authentication for all administrative paths.

- If `y`: Any access to the Web GUI or the core JSON REST API requires a valid username (`admin`) and password verified against NVS storage.
- If `n`: Access control is bypassed. Anyone on the local network can change configuration, read logs, or trigger reboots.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Disabling authentication is highly dangerous for production. If <code>EIF_ENABLE_TLS</code> is disabled, credentials travel in plain text (Base64) and can be easily sniffed out of the air.
</div>

---

`EIF_WEB_SIZE_OTA_BUFFER` - `int` (*range*: `1024` - `65536`, *default*: `4096`)

Name in the `menuconfig`: `OTA update buffer size (bytes)`

Size of the allocation buffer (in bytes) for receiving OTA firmware binaries via HTTP. This buffer size directly impacts throughput and is allocated dynamically from the heap only during an active OTA session.

Recommended values:
- `4096`: Optimal balance between flashing speed and heap preservation.
- `8192` - `16384`: Speeds up flashing significantly if you have free heap available.
- `1024` - `2048`: Minimum possible chunk size for tightly constrained environments.

Setting this value too high may cause OTA initialization to fail due to heap fragmentation (cannot allocate a continuous block).

---

`Logging Settings` -> `EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP` - `bool` (*default*: `n`)

Name in the `menuconfig`: `Include timestamp in HTTP response`

Injects the internal system startup timestamp (`esp_log_timestamp()`) into the body of HTTP responses.

- If `y`: The MCU appends the current uptime in milliseconds as a raw text string to the response body (except for `204 No Content`). This acts as a direct correlation ID. If a specific request fails or triggers an error, you can immediately match the timestamp from the network response with the corresponding line in the UART console logs.
  
  Example output:
  ```
  HTTP/1.1 200 OK
  Content-Type: text/plain
  Content-Length: 5
  Connection: keep-alive

  12345
  ```
- If `n`: The response body remains empty for standard status replies, minimizing network overhead.
  
  Example output:
  ```
  HTTP/1.1 200 OK
  Content-Type: text/plain
  Content-Length: 0
  Connection: keep-alive
  ㅤ
  ```