<!--
  SPDX-FileCopyrightText: 2026 AmakeSasha
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Path: ./docs/REST_API.md
  
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

# REST API

The `esp_iot_framework` provides a comprehensive REST API and static files for device management and monitoring.

## Authentication
All administrative endpoints (all endpoints listed in `MAP URIs`) require HTTP Basic Authentication. To authenticate, include the following HTTP header in each of your requests:
```text
Authorization: Basic [base64_encoded_credentials]
```

> ℹ️ NOTE
> 
> Replace the `[base64_encoded_credentials]` above with your Base64-encoded `admin:[your_password]`. `[your_password]` set via web interface or API (*default*: empty string).

Example of use:
```text
POST /_/wifi/test HTTP/1.1
Host: 192.168.4.1
Authorization: Basic YWRtaW46
Content-Length: 19

{"profile_index":1}
```

## Map URIs

* `/_`
  * `/files` - only if the Kconfig option `CONFIG_EIF_ENABLE_WEB_ADMIN_GUI` is enabled
    * `/logs.html` - The page for viewing logs from the device

      Only if the Kconfig option `CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG` is enabled
      
    * `/` - Main interface container
    * `/api.js` - JavaScript API library
    * `/style.css` - Stylesheet
    * `/system.html` - System management page
    * `/network.html` - Network management page
  * `/wifi`
    * `/list` - [List Wi-Fi profiles](#h_wifi_list_json)
    * `/info` - [Get Wi-Fi and network information](#h_wifi_info_json)
    * `/update` - [Update Wi-Fi profile](#h_wifi_update_do)
    * `/clear` - [Clear Wi-Fi profile](#h_wifi_clear_do)
    * `/test` - [Wi-Fi network availability test](#h_wifi_test_do)
  * `/tls` - only if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled
    * `/recreate` - [Regenerate TLS keys and certificate](#h_tls_recreate_do)
  * `/sys`
    * `/logs` - [Get logs from the device](#h_sys_logs_txt)

      Only if the Kconfig option `CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG` is enabled
    * `/info` - [Get system information](#h_sys_info_json)
    * `/reboot` - [Reboot system](#h_sys_reboot_do)
  * `/ota`
    * `/info` - [Get firmware information](#h_ota_info_json)
    * `/update` - [Upload firmware](#h_ota_update_do)
    * `/confirm` - [Confirm successful update](#h_ota_confirm_do)
    * `/rollback` - [Rollback firmware](#h_ota_rollback_do)
  * `/apass`
    * `/update` - [Update administrator password](#h_apass_update_do)

## API Endpoints

### Wi-Fi Management

<a id="h_wifi_list_json"></a>
List Wi-Fi profiles
```text
GET /_/wifi/list
```

Returns all configured Wi-Fi profiles and current connection status.
  
**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
  ```json
  {
    "current_profile_index": 0,
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
  - `current_profile_index`: Currently active profile (`0` - `CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT`)
  - `rssi_now_profile`: RSSI of current connection in dBm
  - `profiles`: Array of Wi-Fi profiles
    - `ssid`: Network SSID (empty if not configured)
    - `password`: Password (only for profile `0`)
* `500`: **Internal error**


---



<a id="h_wifi_info_json"></a>
Get Wi-Fi and network information
```text
GET /_/wifi/info
```

Returns detailed active network connection parameters.
  
**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
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
    "mdns_hostname": "device-aabbcc"
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
  - `lwip_hostname`: Device network name registered in the router DHCP client table
  - `mdns_hostname`: Local mDNS domain name
* `500`: **Internal error**



---



<a id="h_wifi_update_do"></a>
Update Wi-Fi profile
```text
POST /_/wifi/update
```

Update the values of the existing profile
  
**Request**: 
* **Body**:
  ```json
  {
    "profile_index": 1,
    "ssid": "MyNetwork",
    "password": "MyPassword123"
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`1` - `CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT`, profile `0` cannot be changed)
  - `ssid`: Network SSID (`1` - `31` characters)
  - `password`: Network password (`8` - `63` characters)

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request body**
* `500`: **Internal error**



---



<a id="h_wifi_clear_do"></a>
Clear Wi-Fi profile
```text
POST /_/wifi/clear
```

Clear profile fields with empty values

**Request**:
* **Body**:
  ```json
  {
    "profile_index": 1
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`1` - `CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT`, profile `0` cannot be changed)

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request body**
* `500`: **Internal error**



---



<a id="h_wifi_test_do"></a>
Wi-Fi network availability test
```text
POST /_/wifi/test
```

Checking the availability of a Wi-Fi network using profile

**Request**:
* **Body**:
  ```json
  {
    "profile_index": 1
  }
  ```
  **Fields**:
  - `profile_index`: Profile number (`0` - `CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT`)

**Response**:
* `204`: **Network found**
* `400`: **Invalid request body**
* `422`: **Network not found**
* `500`: **Internal error**



---



### TLS Management

> ℹ️ NOTE
> 
> Only available if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled.

<a id="h_tls_recreate_do"></a>
Regenerate TLS keys and certificate
```text
POST /_/tls/recreate
```

Generates a new self-signed TLS certificate and keys

**Request**: `Nothing`

**Response**:
* `202`: **Successful execution**
* `500`: **Internal error**

> ⚠️ WARNING
> 
> After the TLS credentials is cleared, the device reboots to generate a new set upon startup.



---



### System Management

<a id="h_sys_logs_txt"></a>
Get logs from the device
```
GET /_/sys/logs
```

> ℹ️ NOTE
> 
> Only available if the Kconfig option `CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG` is enabled.

Streams the system log buffer in plain text using HTTP chunked transfer encoding.

**Request**: `Nothing`

**Response**:
* `200`: **Successful execution** (`text/plain; charset=UTF-8`, `chunked`)
  ```text
  [0;32mI (430330) HTTPS server: GET /_/?p=logs.html[0m
  [0;32mI (430330) EIF NVS: [read] operation for key 'eif_basic_auth' completed[0m
  [0;32mI (430330) EIF NVS: 'Basic Auth credentials' loaded[0m
  [0;32mI (430340) HTTPS server: Result(10): ESP_OK[0m
  [0;32mI (430430) HTTPS server: GET /_/files/logs.html[0m
  [0;32mI (430430) EIF NVS: [read] operation for key 'eif_basic_auth' completed[0m
  [0;32mI (430430) EIF NVS: 'Basic Auth credentials' loaded[0m
  [0;32mI (430450) HTTPS server: Result(20): ESP_OK[0m
  [0;32mI (430520) HTTPS server: GET /_/files/api.js[0m
  [0;32mI (430520) EIF NVS: [read] operation for key 'eif_basic_auth' completed[0m
  [0;32mI (430520) EIF NVS: 'Basic Auth credentials' loaded[0m
  [0;32mI (430530) HTTPS server: Result(10): ESP_OK[0m
  [0;32mI (430660) HTTPS server: GET /_/sys/logs?t=1792387492005[0m
  [0;32mI (430660) EIF NVS: [read] operation for key 'eif_basic_auth' completed[0m
  [0;32mI (430660) EIF NVS: 'Basic Auth credentials' loaded[0m
  ```
  Log lines contain ANSI color escape sequences (`ESC[0;32m ... ESC[0m`), as in the UART output.<br><br>
  The device pops logs out of the internal cyclic buffer in chunks of `512` bytes. Chunks are continuously sent to the client until the log buffer is completely empty. If a network write failure occurs mid-stream, transmission aborts immediately to protect system resources.



---



<a id="h_sys_info_json"></a>
Get system information
```text
GET /_/sys/info
```

Returns detailed system information.
  
**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
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

* `500`: **Internal error**



---



<a id="h_sys_reboot_do"></a>
Reboot System
```text
POST /_/sys/reboot
```

Gracefully reboot the device.

**Request**: `Nothing`

**Response**:
* `202`: **Successful execution**



---



### OTA Updates

<a id="h_ota_info_json"></a>
Get firmware information
```text
GET /_/ota/info
```

Returns current firmware information.

**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
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
    "partition": "ota_1",
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
  
  | Status           | Description                        |
  |------------------|------------------------------------|
  | `new`            | Firmware uploaded, not booted yet. |
  | `pending_verify` | First boot. Waiting for confirm.   |
  | `valid`          | Firmware confirmed and stable.     |
  | `invalid`        | Firmware failed checks.            |
  | `aborted`        | Update interrupted.                |
  | `factory`        | Running factory partition.         |
  | `unknown`        | Unknown state.                     |
* `500`: **Internal error**



---



<a id="h_ota_update_do"></a>
Upload firmware
 ```text
POST /_/ota/update
```

Uploads a new firmware binary to the device. Once the upload is complete and verified, the device will automatically reboot to apply the update.

**Request**:
* **Headers**:
  * `Content-Type`: `application/octet-stream`
* **Body**: Raw firmware binary (`.bin` file)

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request body**
* `409`: **OTA update already in progress**
* `500`: **Internal error**



---



<a id="h_ota_confirm_do"></a>
Confirm successful update
```text
POST /_/ota/confirm
```

Marks the current firmware as valid to prevent automatic rollback on the next boot.

**Request**: `Nothing`

**Response**:
* `202`: **Successful execution**
* `500`: **Internal error**

---

<a id="h_ota_rollback_do"></a>
Rollback firmware
```text
POST /_/ota/rollback
```

Rolls back to the previous firmware version. The device will automatically reboot.

**Request**: `Nothing`

**Response**:
* `202`: **Successful execution**
* `500`: **Internal error**



---



### Authentication Management

<a id="h_apass_update_do"></a>
Update administrator password
```text
POST /_/apass/update
```

Changes the administrator password used for Basic Authentication.

**Request**:
* **Body**:
  ```json
  {
    "password": "NewSecurePassword123"
  }
  ```
  **Fields**:
  - `password`: New password (`0` - `32` characters)

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request body**
* `500`: **Internal error**
