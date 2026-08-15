<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_core
  File: KCONFIG.md
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

# Kconfig esp_iot_framework_core

This section describes the `esp_iot_framework_core` configuration parameters. All settings are available in the `idf.py menuconfig` configuration utility within the `ESP IoT framework, CORE` menu.

---

<div class="highlight-block" id="CONFIG_EIF_ENABLE_MDNS">
  <code>EIF_ENABLE_MDNS</code> - <code>bool</code> (<em>default</em>: <code>y</code>)
</div>

Name in the `menuconfig`: `Enable mDNS Support (Multicast DNS)`

Enables the Multicast DNS (`mDNS`) responder service. 

- If `y`: The device can be accessed via both a friendly local name (e.g., `http://device-aabbcc.local`) and its IP (e.g., `http://192.168.1.45`). This makes it easier to find the device without checking the router's DHCP list.
- If `n`: The device is accessible only via its direct IP address (e.g., `http://192.168.1.45`).

---

<div class="highlight-block" id="CONFIG_EIF_ENABLE_TLS">
  <code>EIF_ENABLE_TLS</code> - <code>bool</code> (<em>default</em>: <code>y</code>)
</div>

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

<div class="highlight-block" id="CONFIG_EIF_LOG_LEVEL">
  <code>Logging Settings</code> -> <code>EIF_LOG_LEVEL</code> - <code>int</code> (<em>range</em>: <code>0</code> - <code>4</code>, <em>default</em>: <code>3</code>)
</div>

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

<div class="highlight-block" id="CONFIG_EIF_LOG_SHOW_METADATA">
  <code>Logging Settings</code> -> <code>EIF_LOG_SHOW_METADATA</code> - <code>bool</code> (<em>default</em>: <code>n</code>)
</div>

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

<div class="highlight-block" id="CONFIG_EIF_LOG_ENABLE_MEM_MONITOR">
  <code>Logging Settings</code> -> <code>EIF_LOG_ENABLE_MEM_MONITOR</code> - <code>bool</code> (<em>default</em>: <code>n</code>)
</div>

Name in the `menuconfig`: `Enable memory status logging`

-  If `y`: The monitor task dumps the total free heap, the largest free block, and the critical status flag to the console. This allows tracking memory leaks and fragmentation every <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_MEM_MONITOR_CHECK_INTERVAL</a></code> ms.
  
  Example output:
  ```
  I (34456) memory_monitor: free=184320, largest=96240, critical=NO
  ```

- If `n`: Statistics are not printed. Only warnings (`EIF_LOG_W()`) and critical errors (`EIF_LOG_E()`) will be logged if memory pressure is detected.

---

<div class="highlight-block" id="CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG">
  <code>Logging Settings</code> -> <code>EIF_LOG_ENABLE_REMOTE_DEBUG</code> - <code>bool</code> (<em>default</em>: <code>n</code>)
</div>

Name in the `menuconfig`: `Enable remote diagnostic logging engine`

Intercepts all system and application logs into an internal RAM ring buffer for network-based diagnostics.

- If `y`: The framework redirects all standard console output to a continuous FreeRTOS ring buffer. These logs can be read via the <code><a class="el" href="../../docs/invalid_link.md#group__core__ext__group.html" title="CORE Extension">CORE Extension</a></code> API (`eif_core_log_pop_chunk()`) and transmitted over network protocols (HTTP, MQTT, BLE, or WebSockets), eliminating the need for a physical UART/serial connection.

- If `n`: Logs are routed strictly to the physical UART port. Remote logging APIs are completely compiled out to save flash and RAM.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Enabling this feature requires additional heap memory allocated for the buffer. The buffer size must be configured via <code>EIF_LOG_REMOTE_BUFFER_SIZE</code>.
</div>

---

<div class="highlight-block" id="CONFIG_EIF_LOG_REMOTE_BUFFER_SIZE">
  <code>Logging Settings</code> -> <code>EIF_LOG_REMOTE_BUFFER_SIZE</code> - <code>int</code> (<em>range</em>: <code>1024</code> - <code>51200</code>, <em>default</em>: <code>4096</code>)
</div>

Name in the `menuconfig`: `Remote log ring buffer size (bytes)`

Size of the internal FreeRTOS ring buffer in RAM. `4096` bytes can store around 50-70 log lines. Higher values consume more heap memory.

---

<div class="highlight-block" id="CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">
  <code>Memory Monitor Settings</code> -> <code>EIF_MEM_MONITOR_CRITICAL_SIZE</code> - <code>int</code> (<em>range</em>: <code>12288</code> - <code>65536</code>, <em>default</em>: <code>12288</code>)
</div>

Name in the `menuconfig`: `The minimum allowed size of the largest block`

Minimum free block size (in bytes) that must be available for stable operation. If the largest free block is smaller than this value, the system will reboot. Setting too low may cause missed fragmentation issues; too high may lead to unnecessary reboots.

<div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    To ensure the reboot task can be created, this value <b>MUST</b> be at least <code>8</code> times larger than <code><a href="group__core__kconfig.html#CONFIG_EIF_REBOOT_TASK_STACK_SIZE">EIF_REBOOT_TASK_STACK_SIZE</a></code> (which is measured in 4-byte words).<br><br>
    Formula: <code>CRITICAL_SIZE >= <a href="group__core__kconfig.html#CONFIG_EIF_REBOOT_TASK_STACK_SIZE">EIF_REBOOT_TASK_STACK_SIZE</a> * 8</code>.
</div>

---

<div class="highlight-block" id="CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">
  <code>Memory Monitor Settings</code> -> <code>EIF_MEM_MONITOR_CHECK_INTERVAL</code> - <code>int</code> (<em>range</em>: <code>1000</code> - <code>3600000</code>, <em>default</em>: <code>30000</code>)
</div>

Name in the `menuconfig`: `Memory check interval (ms)`

How often to check memory fragmentation (in milliseconds). This parameter defines the time interval between consecutive memory fragmentation checks performed by the memory monitor task. The actual time to reboot = <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_MEM_MONITOR_CHECK_INTERVAL</a></code> × <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_MEM_MONITOR_NUMBER_CHECKS</a></code>.

Examples (when memory is fragmented):
- <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_MEM_MONITOR_CHECK_INTERVAL</a></code> = `30000` (30 s) with <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_MEM_MONITOR_NUMBER_CHECKS</a></code> = `3` -> reboot after `90` seconds (1.5 min).
- <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL">EIF_MEM_MONITOR_CHECK_INTERVAL</a></code> = `60000` (60 s) with <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">EIF_MEM_MONITOR_NUMBER_CHECKS</a></code> = `5` -> reboot after `300` seconds (5 min).

---

<div class="highlight-block" id="CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS">
  <code>Memory Monitor Settings</code> -> <code>EIF_MEM_MONITOR_NUMBER_CHECKS</code> - <code>int</code> (<em>range</em>: <code>2</code> - <code>60</code>, <em>default</em>: <code>3</code>)
</div>

Name in the `menuconfig`: `Number of fragmentation confirmations before reboot`

Number of consecutive memory fragmentation checks that must detect a critical condition before the system initiates a safe reboot.

This parameter prevents false reboots due to temporary memory spikes. The system will only trigger a reboot if the largest free memory block remains below the critical threshold (<code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">EIF_MEM_MONITOR_CRITICAL_SIZE</a></code>) for the specified number of consecutive checks.

---

<div class="highlight-block" id="CONFIG_EIF_REBOOT_TASK_STACK_SIZE">
  <code>Memory Monitor Settings</code> -> <code>EIF_REBOOT_TASK_STACK_SIZE</code> - <code>int</code> (<em>range</em>: <code>1536</code> - <code>16384</code>, <em>default</em>: <code>1536</code>)
</div>

Name in the `menuconfig`: `Reboot task stack size (words - 4 bytes)`

Stack size for the task that manages the system shutdown sequence.
   
The task stops the Wi-Fi driver halts the HTTP server, and executes the `user_pre_reboot_cb` (registered using the `eif_register_handler_system_reboot()`) before calling `esp_restart()`. Increase this value if `user_pre_reboot_cb` performs stack-heavy operations like NVS writes or complex logging.

Defined in 4-byte words. To ensure the task can be successfully created in low-memory conditions, <code><a href="group__core__kconfig.html#CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE">EIF_MEM_MONITOR_CRITICAL_SIZE</a></code> must be at least `8` times larger than this value.
