<!--
  SPDX-FileCopyrightText: 2026 AmakeSasha
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Path: ./docs/KCONFIG.md
  
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

# Kconfig

This section describes the `esp_iot_framework` configuration parameters. All settings are available in the `idf.py menuconfig` configuration utility within the `ESP IoT framework` menu.

---

<span id="CONFIG_EIF_ENABLE_TLS"></span>
`EIF_ENABLE_TLS` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable HTTPS (TLS/SSL) Support`

Switch the web server from insecure `HTTP` to encrypted `HTTPS` (Port `443`).

- If `y`: All traffic is fully encrypted using `TLS`. This provides a secure communication channel, preventing "Man-in-the-Middle" (MITM) attacks and ensuring that all data remains confidential and integral during transit.
- If `n`: The server operates in plain text mode over `HTTP` (Port `80`). **ALL DATA** sent between the client and the device is visible to anyone on the same network. Using simple packet sniffers, an attacker can easily intercept your session and compromise the system.

`HTTPS` is resource-intensive for the ESP32. Enabling this feature will result in:
* Increased RAM consumption (~30-50 KB for TLS buffers).
* Initial connection latency (1-3 seconds for the TLS handshake).
* Higher CPU utilization during active data transfer.

> ⚠️ WARNING
> 
> Disabling `TLS` (setting to `n`) is acceptable **ONLY** under specific conditions: when the device operates inside a strictly isolated, trusted local network environment, or behind a secure reverse proxy/gateway that handles external encryption. It is strongly discouraged for open production environments, as modern browsers will mark the connection as `Not Secure` and may block access to critical web features.
> 
> Note that even with `TLS` enabled, browsers will still display a warning (e.g., `NET::ERR_CERT_AUTHORITY_INVALID`) for the entire site, including any web pages, resources, or API requests. This happens because the device uses a self-signed certificate from an untrusted authority, which you must manually bypass in the browser to establish a secure session.
> 
> Don't worry, the device generates `TLS` credentials completely autonomously and doesn't share encryption keys with anyone. If necessary, you can request regeneration of `TLS` credentials at any time through the admin panel.

---

<span id="CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT"></span>
`EIF_CUSTOM_WIFI_PROFILES_COUNT` - `int` (*range*: `1` - `7`, *default*: `2`)

Name in the `menuconfig`: `Count custom Wi-Fi profiles`

The number of user-defined Wi-Fi profiles that can be stored in NVS and configured via the Web Panel/API.

> ℹ️ NOTE
> 
> The valid user-accessible profile range starts from index `1` up to `CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT`. Profile index `0` is strictly reserved by the framework as a hardcoded, read-only recovery profile (`SSID: ESP32_SETUP`, `PASS: 12345678`) and cannot be modified or cleared.

---

<span id="CONFIG_EIF_ENABLE_WEB_ADMIN_GUI"></span>
`Web admin GUI` -> `EIF_ENABLE_WEB_ADMIN_GUI` - `bool` (*default*: `y`)

Name in the `menuconfig`: `Enable Web-based Administration Panel`

Enables the integrated web interface for framework management. This affects **ONLY** the administrative panel, not your custom user UI.

- If `y`: All static web assets (HTML, CSS, JS) are compressed and embedded directly into the firmware flash.
- If `n`: The entire Web GUI is stripped from the build. The device will expose only the raw JSON REST API for remote integration.

Disabling this feature significantly reduces Flash usage and optimizes the HTTP server's memory footprint by completely removing the file-serving routing logic.

---

<span id="CONFIG_EIF_LOG_LEVEL"></span>
`Logging Settings` -> `EIF_LOG_LEVEL` - `int` (*range*: `0` - `4`, *default*: `3`)

Name in the `menuconfig`: `Framework log verbosity`

Global logging level for all `esp_iot_framework` modules. Choosing a level enables it and all less verbose levels, while completely turning off more detailed lower levels.

* `0`: `None`
  * Completely turns off all framework logs. Macros `EIF_LOG_*` are compiled out into empty statements (`(void)0U`).
* `1`: `Error` - `EIF_LOG_E()`
  * Turns on only `Errors`. Everything below (`Warning`, `Info`, `Debug`) is turned off.
* `2`: `Warning` - `EIF_LOG_W()`
  * Turns on `Warnings` and `Errors`. `Info` and `Debug` logs are turned off.
* `3`: `Info` - `EIF_LOG_I()`
  * Turns on `Info`, `Warning`, and `Error`. Only `Debug` logs are turned off.
* `4`: `Debug` - `EIF_LOG_D()`
  * Turns on absolutely everything. Full output mode without any filtering.

---

<span id="CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG"></span>
`Logging Settings` -> `EIF_LOG_ENABLE_REMOTE_DEBUG` - `bool` (*default*: `n`)

Name in the `menuconfig`: `Enable remote diagnostic logging engine`

Intercepts all system and application logs into an internal RAM ring buffer for network-based diagnostics.

- If `y`: The framework redirects all standard console output to a continuous FreeRTOS ring buffer using a custom internal `vprintf` interceptor. Because the framework functions as a monolith with an integrated web server, these buffered log chunks are explicitly exposed and transmitted over the network via the built-in system API endpoint: `GET /_/sys/logs`. This allows live monitoring directly in the web admin GUI panel without requiring a physical UART/serial connection.
- If `n`: Logs are routed strictly to the physical UART port. Remote logging APIs and the `/_/sys/logs` route handling logic are completely compiled out to save flash and RAM.

>⚠️ WARNING
>
> Enabling this feature requires additional heap memory allocated for the buffer. The buffer size must be configured via `EIF_LOG_REMOTE_BUFFER_SIZE`.

---

<span id="CONFIG_EIF_LOG_REMOTE_BUFFER_SIZE"></span>
`Logging Settings` -> `EIF_LOG_REMOTE_BUFFER_SIZE` - `int` (*range*: `1024` - `51200`, *default*: `4096`)

Name in the `menuconfig`: `Remote log ring buffer size (bytes)`

Size of the internal FreeRTOS ring buffer in RAM. `4096` bytes can store around 50-70 log lines. Higher values consume more heap memory.

---

<span id="CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE"></span>
`Memory Monitor Settings` -> `EIF_MEM_MONITOR_CRITICAL_SIZE` - `int` (*range*: `12288` - `65536`, *default*: `12288`)

Name in the `menuconfig`: `The minimum allowed size of the largest block`

Minimum free block size (in bytes) that must be available for stable operation. If the largest free block is smaller than this value, the system will initiate a protective reboot sequence. Setting too low may cause stability issues; too high may lead to unnecessary reboots.

> ⚠️ WARNING
> 
> The compilation will explicitly **FAIL** with a preprocessor `#error` if this configuration requirement is violated.
> 
>  Formula requirement: `EIF_MEM_MONITOR_CRITICAL_SIZE >= EIF_REBOOT_TASK_STACK_SIZE * 8`.

---

<span id="CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS"></span>
`Memory Monitor Settings` -> `EIF_MEM_MONITOR_NUMBER_CHECKS` - `int` (*range*: `2` - `60`, *default*: `3`)

Name in the `menuconfig`: `Number of fragmentation confirmations before reboot`

Number of consecutive memory fragmentation checks (sampled every 500ms) that must detect a critical condition before the system triggers a safe reboot.

This parameter prevents false reboots due to temporary memory spikes. The system will only trigger a reboot if the largest free memory block remains below the critical threshold (`EIF_MEM_MONITOR_CRITICAL_SIZE`) for the specified number of consecutive checks.

---

<span id="CONFIG_EIF_REBOOT_TASK_STACK_SIZE"></span>
`Memory Monitor Settings` -> `EIF_REBOOT_TASK_STACK_SIZE` - `int` (*range*: `1536` - `16384` words, *default*: `1536`)

Name in the `menuconfig`: `Reboot task stack size (words - 4 bytes)`

Stack size (in 4-byte words) for the task that manages the system shutdown sequence.
   
The task handles execution delays, invokes the custom user-defined reboot callback `sys_handler_reboot` passed via the `eif_init_config_t` structure, and safely restarts the hardware via `esp_restart()`. Increase this value if your custom reboot callback performs stack-heavy operations.

> ℹ️ NOTE
>
> This parameter is passed directly to `xTaskCreate`, so it is expressed in words (1 word = 4 bytes), not raw bytes. To ensure the task can be successfully created under severe memory constraints, ensure your `EIF_MEM_MONITOR_CRITICAL_SIZE` satisfies the `EIF_MEM_MONITOR_CRITICAL_SIZE >= EIF_REBOOT_TASK_STACK_SIZE * 8` requirement.