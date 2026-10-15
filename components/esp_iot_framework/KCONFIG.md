<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Path: ./components/esp_iot_framework/KCONFIG.md
  
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

# Kconfig esp_iot_framework_core

This section describes the `esp_iot_framework_core` configuration parameters. All settings are available in the `idf.py menuconfig` configuration utility within the `ESP IoT framework, CORE` menu.

---

<span id="CONFIG_EIF_ENABLE_TLS"></span>
`EIF_CORE_ENABLE_TLS` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable HTTPS (TLS/SSL) Support`

Switch the web server from insecure `HTTP` to encrypted `HTTPS` (Port `443`).

- If `y`: All traffic is fully encrypted using `TLS`. This provides a secure communication channel, preventing "Man-in-the-Middle" (MITM) attacks and ensuring that all data remains confidential and integral during transit.
- If `n`: The server operates in plain text mode. **ALL DATA** sent between the client and the device is visible to anyone on the same network. Using simple packet sniffers, an attacker can easily intercept your session and compromise the system.

`HTTPS` is resource-intensive for the ESP32. Enabling this feature will result in:
* Increased RAM consumption (~30-50 KB for TLS buffers).
* Initial connection latency (1-3 seconds for the TLS handshake).
* Higher CPU utilization during active data transfer.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Disabling <code>TLS</code> is strongly discouraged for production environments. Modern browsers will mark the connection as <code>Not Secure</code> and may block access to critical web features.<br><br>
    Note that even with <code>TLS</code> enabled, browsers will still display a warning (e.g., <code>NET::ERR_CERT_AUTHORITY_INVALID</code>) for the entire site, including any web pages, resources, or API requests. This happens because the device uses a self-signed certificate from an untrusted authority, which you must manually bypass in the browser to establish a secure session.<br><br>
    Don't worry, the device generates <code>TLS</code> credentials completely autonomously and doesn't share encryption keys with anyone. If necessary, you can request regeneration of <code>TLS</code> credentials at any time through the admin panel.
</div>

---

<span id="CONFIG_EIF_ENABLE_BASIC_AUTH"></span>
`EIF_CORE_ENABLE_BASIC_AUTH` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable Basic Authentication for systemic resources`

Enables mandatory HTTP Basic Authentication for secure system-level resources, administrative endpoints, and protected infrastructure nodes.

- If `y`: Any access to system REST APIs, administrative paths, or managed assets requires a valid username (`admin`) and password verified against NVS storage.
- If `n`: Access control for infrastructure layers is completely bypassed. Anyone on the local network can view telemetry, modify configurations, read logs, or trigger reboots.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Disabling authentication is highly dangerous for production. If <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">EIF_CORE_ENABLE_TLS</a></code> is disabled, credentials travel in plain text (Base64) and can be easily sniffed out of the air.
</div>

---

<span id="CONFIG_EIF_LOG_LEVEL"></span>
`Logging Settings` -> `EIF_CORE_LOG_LEVEL` - `int` (*range*: `0` - `4`, *default*: `3`)

Name in the `menuconfig`: `Framework log verbosity`

Global logging level for all `esp_iot_framework` modules. Choosing a level enables it and all less verbose levels, while completely turning off more detailed lower levels.

* `0`: `None`
  * Turns off absolutely everything. No errors, warnings, or system logs will be printed to the console.
* `1`: `Error` - `EIF_LOG_E()`
  * Turns on only `Errors`. Everything below (`Warning`, `Info`, `Debug`) is turned off.
* `2`: `Warning` - `EIF_LOG_W()`
  * Turns on `Warnings` and `Errors`. `Info` and `Debug` logs are turned off.
* `3`: `Info` - `EIF_LOG_I()`
  * Turns on `Info`, `Warning`, and `Error`. Only `Debug` logs are turned off.
* `4`: `Debug` - `EIF_LOG_D()`
  * Turns on absolutely everything. Full output mode without any filtering.

---

<span id="CONFIG_EIF_LOG_SHOW_METADATA"></span>
`Logging Settings` -> `EIF_CORE_LOG_SHOW_METADATA` - `bool` (*default*: `n`)

Name in the `menuconfig`: `Show metadata in logs (path, line, function)`

Show metadata in logs (path, line, function).

- If `y`:
  ```
  I (8536) HTTPS server: src/web.c:990 (eif_server_launch) HTTP Server started.
  ```
- If `n`:
  ```
  I (8536) HTTPS server: HTTP Server started.
  ```

---

<span id="CONFIG_EIF_LOG_ENABLE_MEM_MONITOR"></span>
`Logging Settings` -> `EIF_CORE_LOG_ENABLE_MEM_MONITOR` - `bool` (*default*: `n`)

Name in the `menuconfig`: `Enable memory status logging`

-  If `y`: The monitor task dumps the total free heap, the largest free block, and the critical status flag to the console. This allows tracking memory leaks and fragmentation every <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_CORE_MEM_MONITOR_CHECK_INTERVAL</a></code> ms.
  
  Example output:
  ```
  I (34456) memory_monitor: free=184320, largest=96240, critical=NO
  ```

- If `n`: Statistics are not printed. Only warnings (`EIF_CORE_LOG_W()`) and critical errors (`EIF_CORE_LOG_E()`) will be logged if memory pressure is detected.

---

<span id="CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG"></span>
`Logging Settings` -> `EIF_CORE_LOG_ENABLE_REMOTE_DEBUG` - `bool` (*default*: `n`)

Name in the `menuconfig`: `Enable remote diagnostic logging engine`

Intercepts all system and application logs into an internal RAM ring buffer for network-based diagnostics.

- If `y`: The framework redirects all standard console output to a continuous FreeRTOS ring buffer. These logs can be read via the <code><a class="el" href="../../docs/invalid_link.md" real_ref="group__core__ext__group.html" title="CORE Extension">CORE Extension</a></code> API (`eif_core_log_pop_chunk()`) and transmitted over network protocols (HTTP, MQTT, BLE, or WebSockets), eliminating the need for a physical UART/serial connection.

- If `n`: Logs are routed strictly to the physical UART port. Remote logging APIs are completely compiled out to save flash and RAM.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Enabling this feature requires additional heap memory allocated for the buffer. The buffer size must be configured via <code>EIF_CORE_LOG_REMOTE_BUFFER_SIZE</code>.
</div>

---

<span id="CONFIG_EIF_LOG_REMOTE_BUFFER_SIZE"></span>
`Logging Settings` -> `EIF_CORE_LOG_REMOTE_BUFFER_SIZE` - `int` (*range*: `1024` - `51200`, *default*: `4096`)

Name in the `menuconfig`: `Remote log ring buffer size (bytes)`

Size of the internal FreeRTOS ring buffer in RAM. `4096` bytes can store around 50-70 log lines. Higher values consume more heap memory.

---

<span id="CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE"></span>
`Memory Monitor Settings` -> `EIF_CORE_MEM_MONITOR_CRITICAL_SIZE` - `int` (*range*: `12288` - `65536`, *default*: `12288`)

Name in the `menuconfig`: `The minimum allowed size of the largest block`

Minimum free block size (in bytes) that must be available for stable operation. If the largest free block is smaller than this value, the system will reboot. Setting too low may cause missed fragmentation issues; too high may lead to unnecessary reboots.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    To ensure the reboot task can be created, this value <b>MUST</b> be at least <code>8</code> times larger than <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_REBOOT_TASK_STACK_SIZE">EIF_CORE_REBOOT_TASK_STACK_SIZE</a></code> (which is measured in 4-byte words).<br><br>
    Formula: <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">EIF_CORE_MEM_MONITOR_CRITICAL_SIZE</a> >= <a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_REBOOT_TASK_STACK_SIZE">EIF_CORE_REBOOT_TASK_STACK_SIZE</a> * 8</code>.
</div>

---

<span id="CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL"></span>
`Memory Monitor Settings` -> `EIF_CORE_MEM_MONITOR_CHECK_INTERVAL` - `int` (*range*: `1000` - `3600000`, *default*: `30000`)

Name in the `menuconfig`: `Memory check interval (ms)`

How often to check memory fragmentation (in milliseconds). This parameter defines the time interval between consecutive memory fragmentation checks performed by the memory monitor task. The actual time to reboot = <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_CORE_MEM_MONITOR_CHECK_INTERVAL</a></code> × <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_CORE_MEM_MONITOR_NUMBER_CHECKS</a></code>.

Examples (when memory is fragmented):
- <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_CORE_MEM_MONITOR_CHECK_INTERVAL</a></code> = `30000` (30 s) with <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_CORE_MEM_MONITOR_NUMBER_CHECKS</a></code> = `3` -> reboot after `90` seconds (1.5 min).
- <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_CORE_MEM_MONITOR_CHECK_INTERVAL</a></code> = `60000` (60 s) with <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_CORE_MEM_MONITOR_NUMBER_CHECKS</a></code> = `5` -> reboot after `300` seconds (5 min).

---

<span id="CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS"></span>
`Memory Monitor Settings` -> `EIF_CORE_MEM_MONITOR_NUMBER_CHECKS` - `int` (*range*: `2` - `60`, *default*: `3`)

Name in the `menuconfig`: `Number of fragmentation confirmations before reboot`

Number of consecutive memory fragmentation checks that must detect a critical condition before the system initiates a safe reboot.

This parameter prevents false reboots due to temporary memory spikes. The system will only trigger a reboot if the largest free memory block remains below the critical threshold (<code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">EIF_CORE_MEM_MONITOR_CRITICAL_SIZE</a></code>) for the specified number of consecutive checks.

---

<span id="CONFIG_EIF_REBOOT_TASK_STACK_SIZE"></span>
`Memory Monitor Settings` -> `EIF_CORE_REBOOT_TASK_STACK_SIZE` - `int` (*range*: `1536` - `16384`, *default*: `1536`)

Name in the `menuconfig`: `Reboot task stack size (words - 4 bytes)`

Stack size for the task that manages the system shutdown sequence.
   
The task stops the Wi-Fi driver halts the HTTP server, and executes the `handler_system_reboot` (registered using the `eif_register_handler_system_reboot()`) before calling `esp_restart()`. Increase this value if `handler_system_reboot` performs stack-heavy operations like NVS writes or complex logging.

Defined in 4-byte words. To ensure the task can be successfully created in low-memory conditions, <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">EIF_CORE_MEM_MONITOR_CRITICAL_SIZE</a></code> must be at least `8` times larger than this value.

---


<div class="highlight-block" id="CONFIG_EIF_ENABLE_WEB_ADMIN_GUI">
  <code>Web admin GUI</code> -> <code>EIF_SERVER_ENABLE_WEB_ADMIN_GUI</code> - <code>bool</code> (<em>default</em>: <code>y</code>)
</div>

Name in the `menuconfig`: `Enable Web-based Administration Panel`

Enables the integrated web interface for framework management. This affects **ONLY** the administrative panel, not your custom user UI.

- If `y`: All static web assets (HTML, CSS, JS) from the `web_src` directory are compressed and embedded directly into the firmware flash.
- If `n`: The entire Web GUI is stripped from the build. The device will expose only the raw JSON REST API for remote integration.

Disabling this feature significantly reduces Flash usage and optimizes the HTTP server's memory footprint by completely removing the file-serving routing logic.

---

<div class="highlight-block" id="CONFIG_EIF_ENABLE_WEB_FAVICON">
  <code>Web admin GUI</code> -> <code>EIF_SERVER_ENABLE_WEB_FAVICON</code> - <code>bool</code> (<em>default</em>: <code>y</code>)
</div>

Name in the `menuconfig`: `Embed Web Favicon (Logo)`

Includes the official framework branding icon in the firmware.

- If `y`: The PNG favicon image is compiled into Flash and automatically served on `/favicon.ico` requests.
- If `n`: Strips the asset from the firmware, saving extra Flash space and removing the specific URI route handler.

---

<div class="highlight-block" id="CONFIG_EIF_WEB_CACHE_MAX_AGE">
  <code>Web admin GUI</code> -> <code>EIF_SERVER_WEB_CACHE_MAX_AGE</code> - <code>int</code> (<em>range</em>: <code>0</code> - <code>31536000</code>, <em>default</em>: <code>360</code>)
</div>

Name in the `menuconfig`: `Static files cache max-age (seconds)`

Sets the `max-age` value (in seconds) for the HTTP `Cache-Control` header. During this period, the browser serves web pages strictly from its local cache without hitting the ESP32, minimizing CPU and heap usage.

- Production: Set a high value (e.g., `86400`) to avoid repetitive TLS handshakes and save MCU resources.
- Development: Set strictly to `0`. Otherwise, browser caching will hide your web source modifications until a hard refresh (`Ctrl` + `F5`) is forced.

<div style="background-color: #d1ecf1; border-left: 5px solid #17a2b8; padding: 12px; margin: 10px 0; color: #0c5460;">
    <strong>Note</strong><br>
    <code>ETag</code> validation occurs only <b>AFTER</b> this timer expires or if the user performs a manual refresh.
</div>

---

<div class="highlight-block" id="CONFIG_EIF_WEB_SIZE_OTA_BUFFER">
  <code>EIF_SERVER_WEB_SIZE_OTA_BUFFER</code> - <code>int</code> (<em>range</em>: <code>1024</code> - <code>65536</code>, <em>default</em>: <code>4096</code>)
</div>

Name in the `menuconfig`: `OTA update buffer size (bytes)`

Size of the allocation buffer (in bytes) for receiving OTA firmware binaries via HTTP. This buffer size directly impacts throughput and is allocated dynamically from the heap only during an active OTA session.

Recommended values:
- `4096`: Optimal balance between flashing speed and heap preservation.
- `8192` - `16384`: Speeds up flashing significantly if you have free heap available.
- `1024` - `2048`: Minimum possible chunk size for tightly constrained environments.

Setting this value too high may cause OTA initialization to fail due to heap fragmentation (cannot allocate a continuous block).

---

<div class="highlight-block" id="CONFIG_EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP">
  <code>Logging Settings</code> -> <code>EIF_SERVER_LOG_ENABLE_WEB_SEND_TIMESTAMP</code> - <code>bool</code> (<em>default</em>: <code>n</code>)
</div>

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