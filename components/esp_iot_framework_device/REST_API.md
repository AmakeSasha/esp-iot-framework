<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./components/esp_iot_framework_device
  File: REST_API.md
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

The `esp_iot_framework_device` provides a comprehensive REST API and static files for device management and monitoring. All API endpoints are prefixed with `/_/` and support JSON format for requests and responses.

## Authentication
When Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__device__kconfig.html#CONFIG_EIF_ENABLE_BASIC_AUTH">CONFIG_EIF_ENABLE_BASIC_AUTH</a></code> is enabled, all administrative endpoints (all endpoints listed in `MAP URIs`) require HTTP Basic Authentication.

To authenticate, include the following HTTP header in each of your requests:
```text
Authorization: Basic <base64_encoded_credentials>
```

<div style="background-color: #e2f0fe; border-left: 5px solid #0066cc; padding: 12px; margin: 10px 0; color: #004085;">
  <strong>Note</strong><br>
  Replace the `<base64_encoded_credentials>` above with your Base64-encoded `admin:<your_password>`. `<your_password>` set via web interface or API (default: empty string).
</div>

Example of use:
```text
GET /_/wifi/result.json HTTP/1.1
Host: 192.168.4.1
Authorization: Basic YWRtaW46
Content-Length: 20

{"profile_index": 1}
```

## Map URIs

* `/_`
  * `/files` - only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__device__kconfig.html#CONFIG_EIF_ENABLE_WEB_ADMIN_GUI">CONFIG_EIF_ENABLE_WEB_ADMIN_GUI</a></code> is enabled
    * `/favicon.ico` - Project logo
    
      Only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__device__kconfig.html#CONFIG_EIF_ENABLE_WEB_FAVICON">CONFIG_EIF_ENABLE_WEB_FAVICON</a></code> is enabled
    * `/logs.html` - The page for viewing logs from the device

      Only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG">CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG</a></code> is enabled
    * `/api.js` - JavaScript API library
    * `/json2.js` - JSON utility library
    * `/style.css` - Stylesheet
    * `/license.txt` - License information
    * `/index.html` - Main interface container
    * `/system.html` - System management page
    * `/network.html` - Network management page
  * `/wifi`
    * `/list.json` - [List Wi-Fi profiles](#h_wifi_list_json)
    * `/info.json` - [Get Wi-Fi and network information](#h_wifi_info_json)
    * `/update.do` - [Update Wi-Fi profile](#h_wifi_update_do)
    * `/clear.do` - [Clear Wi-Fi profile](#h_wifi_clear_do)
    * `/check.do` - [Wi-Fi network availability test](#h_wifi_check_do)
    * `/result.json` - [Get the results of the Wi-Fi test](#h_wifi_result_json)
  * `/tls` - only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">CONFIG_EIF_ENABLE_TLS</a></code> is enabled
    * `/recreate.do` - [Regenerate TLS keys and certificate](#h_tls_recreate_do)
  * `/sys`
    * `/logs.txt` - [Get logs from the device](#h_sys_logs_txt)

      Only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG">CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG</a></code> is enabled
    * `/info.json` - [Get system information](#h_sys_info_json)
    * `/reboot.do` - [Reboot system](#h_sys_reboot_do)
  * `/ota`
    * `/info.json` - [Get firmware information](#h_ota_info_json)
    * `/update.do` - [Upload firmware](#h_ota_update_do)
    * `/confirm.do` - [Confirm successful update](#h_ota_confirm_do)
    * `/rollback.do` - [Rollback firmware](#h_ota_rollback_do)
  * `/apass` - only if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__device__kconfig.html#CONFIG_EIF_ENABLE_BASIC_AUTH">CONFIG_EIF_ENABLE_BASIC_AUTH</a></code> is enabled
    * `/update.do` - [Update administrator password](#h_apass_update_do)

## HTTP Status Codes

- `200 OK` - Successful GET request
- `202 Accepted` - Action accepted and processing
- `204 No Content` - Successful POST
- `400 Bad Request` - Invalid request parameters
- `401 Unauthorized` - Authentication required/failed
- `404 Not Found` - Resource not found
- `409 Conflict` - Resource conflict (e.g., OTA already in progress)
- `500 Internal Server Error` - Server error

## API Endpoints

### Wi-Fi Management

<a id="h_wifi_list_json"></a>
* List Wi-Fi profiles
  ```text
  GET /_/wifi/list.json
  ```

  Returns all configured Wi-Fi profiles and current connection status.
  
  **Request body**: `No body`

  **Response body**:
  ```json
  {
    "current_profile_index": 0,
    "used_tls": true,
    "rssi_now_profile": -65,
    "profiles": [
      {
        "ssid": "ESP32_SETUP",
        "password": "12345678"
      },
      {
        "ssid": "OfficeWiFi"
      },
      {
        "ssid": ""
      }
    ]
  }
  ```
  **Fields**:
  - `current_profile_index`: Currently active profile (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)
  - `used_tls`: Whether TLS is enabled on the server or not
  - `rssi_now_profile`: RSSI of current connection in dBm
  - `profiles`: Array of Wi-Fi profiles
    - `ssid`: Network SSID (empty if not configured)
    - `password`: Password (only for profile `0`)

---

<a id="h_wifi_info_json"></a>
* Get Wi-Fi and network information
  ```text
  GET /_/wifi/info.json
  ```

  Returns detailed active network connection parameters.
  
  **Request body**: `No body`

  **Response body**:
  ```json
  {
    "ssid": "MyNetwork",
    "rssi": -65,
    "channel": 6,
    "auth_mode": "WPA2_PSK",
    "bssid": "00:11:22:33:44:55",
    "ip": "192.168.1.45",
    "gateway": "192.168.1.1",
    "netmask": "255.255.255.0",
    "mac": "AA:BB:CC:DD:EE:FF",
    "proto": "802.11n (Wi-Fi 4)",
    "band": "2.4 GHz",
    "lwip_hostname": "device-aabbcc",
    "mdns_hostname": "device-aabbcc",
    "used_tls": true
  }
  ```
  **Fields**:
  - `ssid`: Network SSID of the current active connection
  - `rssi`: RSSI of current connection in dBm
  - `channel`: Radio channel used by the current network
  - `auth_mode`: Security type (string name mapped from `wifi_auth_mode_t`)
  - `bssid`: Physical MAC address of the connected router antenna
  - `ip`: Device IPv4 address assigned to the station interface
  - `gateway`: Default gateway IP address
  - `netmask`: Subnet mask
  - `mac`: Station hardware MAC address of the device (`ESP_MAC_WIFI_STA`)
  - `proto`: Wi-Fi protocol generation (mapped from `esp_wifi_get_protocol()`)
  - `band`: Active frequency band (`2.4 GHz` or `5 GHz`)
  - `lwip_hostname`: Device network name registered in the router DHCP client table (`null` if unassigned)
  - `mdns_hostname`: Local mDNS domain name (`null` if <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_ENABLE_MDNS">CONFIG_EIF_ENABLE_MDNS</a></code> is disabled)
  - `used_tls`: Whether TLS is enabled on the server or not

---

<a id="h_wifi_update_do"></a>
* Update Wi-Fi profile
  ```text
  POST /_/wifi/update.do
  ```

  Update the values of the existing profile
  
  **Request body**:
  ```json
  {
    "profile_index": 1,
    "ssid": "MyNetwork",
    "password": "MyPassword123"
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`1` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>, profile `0` cannot be changed)
  - `ssid`: Network SSID (<code>#EIF_WIFI_SSID_MIN_LEN</code> - <code>#EIF_WIFI_SSID_MAX_LEN - 1</code> characters)
  - `password`: Network password (<code>#EIF_WIFI_PASS_MIN_LEN</code> - <code>#EIF_WIFI_PASS_MAX_LEN - 1</code> characters)

  **Response**: `HTTP 204 No Content`

---

<a id="h_wifi_clear_do"></a>
* Clear Wi-Fi profile
  ```text
  POST /_/wifi/clear.do
  ```

  Clear profile fields with empty values

  **Request Body**:
  ```json
  {
    "profile_index": 1
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (Profile number (`1` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>), profile #0 cannot be changed)

  **Response**: `HTTP 204 No Content`

---

<a id="h_wifi_check_do"></a>
* Wi-Fi network availability test
  ```text
  POST /_/wifi/check.do
  ```

  Checking the availability of a Wi-Fi network using profile

  **Request Body**:
  ```json
  {
    "profile_index": 1
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)

  **Response**: `HTTP 202 Accepted`

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    This operation can take quite a long time (tens of seconds). During the test, the ESP also disconnects from the actual Wi-Fi network and attempts to connect to the network being tested. This is done due to the physical limitations of the ESP.
  </div>

---

<a id="h_wifi_result_json"></a>
* Get the results of the Wi-Fi test
  ```text
  POST /_/wifi/result.json
  ```
  <div style="background-color: #e2f0fe; border-left: 5px solid #0066cc; padding: 12px; margin: 10px 0; color: #004085;">
    <strong>Note</strong><br>
    The use of `POST` is due to the fact that `Internet Explorer 8` does not support sending a `GET` request with a body.
  </div>

  Get the results of the Wi-Fi network availability test

  **Request Body**:
  ```json
  {
    "profile_index": 1
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)

  **Response body**:
  ```json
  {
    "result": true,
    "rssi": -72
  }
  ```
  **Fields**:
  - `result`: `true` if a connection was established and an IP address was obtained, `false` otherwise
  - `rssi`: Signal strength in dBm

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    Due to the change of Wi-Fi networks for the test, this request may be sent too early and the server may not even see it. To get the result, use a loop of requests after some time has passed since the test started (about 8 seconds)
  </div>

---

### TLS Management

<div style="background-color: #e2f0fe; border-left: 5px solid #0066cc; padding: 12px; margin: 10px 0; color: #004085;">
  <strong>Note</strong><br>
  Only available if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">CONFIG_EIF_ENABLE_TLS</a></code> is enabled.
</div>

<a id="h_tls_recreate_do"></a>
* Regenerate TLS keys and certificate
  ```text
  POST /_/tls/recreate.do
  ```

  Generates a new self-signed TLS certificate and keys

  **Request Body**: `No body`

  **Response**: `HTTP 202 Accepted`

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    After regenerating TLS keys and certificate, the device will automatically restart to apply the new configuration. The entire process usually takes less than 5 seconds.
  </div>

---

### System Management

<a id="h_sys_logs_txt"></a>
* Get logs from the device
  ```
  GET /_/sys/logs.txt
  ```

  <div style="background-color: #e2f0fe; border-left: 5px solid #0066cc; padding: 12px; margin: 10px 0; color: #004085;">
    <strong>Note</strong><br>
    Only available if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__core__kconfig.html#CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG">CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG</a></code> is enabled.
  </div>

  Streams the system log buffer in plain text using HTTP chunked transfer encoding.

  **Request body**: `No body`

  **Response body**: `Plain text (Log lines separated by \n)`

  **HTTP Headers**:
  - `Content-Type`: `text/plain`
  - `Transfer-Encoding`: `chunked`

  **Response behavior**: The device pops logs out of the internal cyclic buffer in chunks of `HTTP_LOGS_CHUNK_SIZE` bytes. Chunks are continuously sent to the client until the log buffer is completely empty. If a network write failure occurs mid-stream, transmission aborts immediately to protect system resources.

---

<a id="h_sys_info_json"></a>
* Get system information
  ```text
  GET /_/sys/info.json
  ```

  Returns detailed system information.
  
  **Request body**: `No body`

  **Response body**:
  ```json
  {
    "heap_free": 123456,
    "heap_min": 120000,
    "largest_block": 80000,
    "uptime": 3600,
    "cores": 2,
    "chip_rev": 3,
    "flash_size": 4,
    "cpu_freq": 240,
    "chip_model": "ESP32",
    "reset_reason": 1,
    "reset_reason_str": "Power-On Reset",
    "features": {
      "has_wifi": true,
      "has_bluetooth": true,
      "has_ble": true
    },
    "mac": "AA:BB:CC:DD:EE:FF"
  }
  ```
  **Fields**:
  - `heap_free`: Current free heap in bytes
  - `heap_min`: Minimum free heap since boot
  - `largest_block`: Largest contiguous free memory block in heap
  - `uptime`: System uptime in seconds
  - `cores`: Number of CPU cores
  - `chip_rev`: Chip revision
  - `flash_size`: Flash size in MB
  - `cpu_freq`: CPU frequency in MHz
  - `chip_model`: ESP chip model
  - `reset_reason`: Last reset reason code
  - `reset_reason_str`: Last reset reason string
  - `features`: Object containing supported hardware features
    - `has_wifi`: `true` if Wi-Fi (802.11 b/g/n) is supported
    - `has_bluetooth`: `true` if Bluetooth Classic is supported
    - `has_ble`: `true` if Bluetooth Low Energy (BLE) is supported
  - `mac`: Device MAC address

  <br>

  **Reset Reason Codes**:
  | Code | String                     |
  |------|----------------------------|
  | `1`  | `Power-on reset`           |
  | `2`  | `External pin reset`       |
  | `3`  | `Software reset`           |
  | `4`  | `System panic reset`       |
  | `5`  | `Watchdog timer reset`     |
  | `6`  | `Task watchdog reset`      |
  | `7`  | `Interrupt watchdog reset` |
  | `8`  | `Deep sleep wake-up reset` |
  | `9`  | `SDIO reset`               |
  | `10` | `USB reset`                |
  | `11` | `JTAG reset`               |
  | `12` | `RTC system reset`         |
  | `13` | `RTC CPU reset`            |

---

<a id="h_sys_reboot_do"></a>
* Reboot System
  ```text
  POST /_/sys/reboot.do
  ```

  Gracefully reboot the device.

  **Request body**: `No body`

  **Response**: `HTTP 202 Accepted`

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    The device will reboot immediately after this request. All active connections will be closed.
  </div>

---

### OTA Updates

<a id="h_ota_info_json"></a>
* Get firmware information
  ```text
  GET /_/ota/info.json
  ```

  Returns current firmware information.

  **Request body**: `No body`

  **Response body**:
  ```json
  {
    "project": "my_iot_project",
    "version": "1.0.0",
    "elf_sha256": "a1b2c3d4e5f6...",
    "build_date": "Jan 1 2026",
    "build_time": "12:00:00",
    "idf_version": "v4.4.6",
    "compiler": "gcc 8.4.0",
    "target": "esp32",
    "partition": "factory",
    "ota_status": "valid"
  }
  ```
  **Fields**:
  - `project`: Project name
  - `version`: Firmware version
  - `elf_sha256`: SHA256 of ELF file
  - `build_date`: Build date
  - `build_time`: Build time
  - `idf_version`: ESP-IDF version
  - `compiler`: Compiler version
  - `target`: Target chip
  - `partition`: Current running partition
  - `ota_status`: OTA status. Possible values:
  <br><table class="doxtable">
    <tr><th>Status</th><th>Description</th></tr>
    <tr><td><code>new</code></td><td>Firmware uploaded, not booted yet.</td></tr>
    <tr><td><code>pending_verify</code></td><td>First boot. Waiting for confirm.do.</td></tr>
    <tr><td><code>valid</code></td><td>Firmware confirmed and stable.</td></tr>
    <tr><td><code>invalid</code></td><td>Firmware failed checks.</td></tr>
    <tr><td><code>aborted</code></td><td>Update interrupted.</td></tr>
    <tr><td><code>factory</code></td><td>Running factory partition.</td></tr>
  </table>


---

<a id="h_ota_update_do"></a>
* Upload firmware
  ```text
  POST /_/ota/update.do
  ```

  Uploads a new firmware binary to the device. Once the upload is complete and verified, the device will automatically reboot to apply the update.

  **Request**:
  - Header `Content-Type`: `application/octet-stream`
  - Body: Raw firmware binary (`.bin` file)

  **Response**: `HTTP 202 Accepted`

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    The device will automatically restart after a successful upload. This process usually takes less than 5 seconds.
  </div>

---

<a id="h_ota_confirm_do"></a>
* Confirm successful update
  ```text
  POST /_/ota/confirm.do
  ```

  Marks the current firmware as valid to prevent automatic rollback on the next boot.

  **Request body**: `No body`

  **Response**: `HTTP 202 Accepted`

---

<a id="h_ota_rollback_do"></a>
* Rollback firmware
  ```text
  POST /_/ota/rollback.do
  ```

  Rolls back to the previous firmware version. The device will automatically reboot.

  **Request body**: `No body`

  **Response**: `HTTP 202 Accepted`

  <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
    The device will automatically restart immediately after the rollback is initiated. This process usually takes less than 5 seconds.
  </div>

---

### Authentication Management

<div style="background-color: #e2f0fe; border-left: 5px solid #0066cc; padding: 12px; margin: 10px 0; color: #004085;">
  <strong>Note</strong><br>
  Only available if the Kconfig option <code><a href="../../docs/invalid_link.md" real_ref="group__device__kconfig.html#CONFIG_EIF_ENABLE_BASIC_AUTH">CONFIG_EIF_ENABLE_BASIC_AUTH</a></code> is enabled.
</div>

<a id="h_apass_update_do"></a>
* Update administrator password
  ```text
  POST /_/apass/update.do
  ```

  Changes the administrator password used for Basic Authentication.

  **Request body**:
  ```json
  {
    "password": "NewSecurePassword123"
  }
  ```
  **Fields**:
  - `password`: New password (<code>#EIF_BASIC_AUTH_PASS_MIN_LEN</code> - <code>#EIF_BASIC_AUTH_PASS_MAX_LEN - 1</code> characters)

  **Response**: `HTTP 204 No Content`