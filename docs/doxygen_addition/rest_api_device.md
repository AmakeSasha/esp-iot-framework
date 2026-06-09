# REST API Device
<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp_iot_framework
  Folder: docs
  File: rest_api_device.md
  
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
When Kconfig option `CONFIG_EIF_ENABLE_BASIC_AUTH` is enabled, all administrative endpoints (all endpoints listed in `MAP URIs`) require HTTP Basic Authentication.

To authenticate, include the following HTTP header in each of your requests:
@code{text}
Authorization: Basic <base64_encoded_credentials>
@endcode

@note Replace the `<base64_encoded_credentials>` above with your Base64-encoded `admin:<your_password>`. `<your_password>` set via web interface or API (default: empty string).

Example of use:
@code{text}
GET /_/wifi/result.json HTTP/1.1
Host: 192.168.4.1
Authorization: Basic YWRtaW46
Content-Length: 20

{"profile_index": 1}
@endcode

## Map URIs

* `/_`
    * `/files` - only if the Kconfig option `CONFIG_EIF_ENABLE_WEB_ADMIN_GUI` is enabled
        * `/index.html` - Main interface container
        * `/network.html` - Network management page
        * `/system.html` - System management page
        * `/style.css` - Stylesheet
        * `/api.js` - JavaScript API library
        * `/json2.js` - JSON utility library
        * `/license.txt` - License information
    * `/wifi`
        * `/list.json` - [List Wi-Fi profiles](@ref h_wifi_list_json)
        * `/update.do` - [Update Wi-Fi profile](@ref h_wifi_update_do)
        * `/clear.do` - [Clear Wi-Fi profile](@ref h_wifi_clear_do)
        * `/check.do` - [Wi-Fi network availability test](@ref h_wifi_check_do)
        * `/result.json` - [Get the results of the Wi-Fi test](@ref h_wifi_result_json)
    * `/tls` - only if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled
        * `/recreate.do` - [Regenerate TLS keys and certificate](@ref h_tls_recreate_do)
    * `/sys`
        * `/info.json` - [Get system information](@ref h_sys_info_json)
        * `/reboot.do` - [Reboot system](@ref h_sys_reboot_do)
   	* `/ota`
        * `/info.json` - [Get firmware information](@ref h_ota_info_json)
        * `/update.do` - [Upload firmware](@ref h_ota_update_do)
        * `/confirm.do` - [Confirm successful update](@ref h_ota_confirm_do)
        * `/rollback.do` - [Rollback firmware](@ref h_ota_rollback_do)
   	* `/apass` - only if the Kconfig option `CONFIG_EIF_ENABLE_BASIC_AUTH` is enabled
        * `/update.do` - [Update administrator password](@ref h_apass_update_do)

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

@anchor h_wifi_list_json
* List Wi-Fi profiles
    @code{text}
    GET /_/wifi/list.json
    @endcode

    Returns all configured Wi-Fi profiles and current connection status.
   
    **Request body**: `No body`

    **Response body**:
    @code{json}
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
    @endcode
    **Fields**:
    - `current_profile_index`: Currently active profile (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>@ref EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)
    - `rssi_now_profile`: RSSI of current connection in dBm
    - `profiles`: Array of Wi-Fi profiles
      - `ssid`: Network SSID (empty if not configured)
      - `password`: Password (only for profile `0`)

---
@anchor h_wifi_update_do
* Update Wi-Fi profile
    @code{text}
    POST /_/wifi/update.do
    @endcode

    Update the values of the existing profile
   
    **Request body**:
    @code{json}
    {
      "profile_index": 1,
      "ssid": "MyNetwork",
      "password": "MyPassword123"
    }
    @endcode
    **Fields**:
    - `profile_index`: Profile number (`1` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>@ref EIF_WIFI_PROFILES_DEFAULT_COUNT</code>, profile `0` cannot be changed)
    - `ssid`: Network SSID (<code>@ref EIF_WIFI_SSID_MIN_LEN</code> - <code>@ref EIF_WIFI_SSID_MAX_LEN - 1</code> characters)
    - `password`: Network password (<code>@ref EIF_WIFI_PASS_MIN_LEN</code> - <code>@ref EIF_WIFI_PASS_MAX_LEN - 1</code> characters)

    **Response**: `HTTP 204 No Content`

---
@anchor h_wifi_clear_do
* Clear Wi-Fi profile
    @code{text}
    POST /_/wifi/clear.do
    @endcode

    Clear profile fields with empty values

    **Request Body**:
    @code{json}
    {
      "profile_index": 1
    }
    @endcode
    **Fields**:
    - `profile_index`: Profile number (Profile number (`1` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>@ref EIF_WIFI_PROFILES_DEFAULT_COUNT</code>), profile #0 cannot be changed)

    **Response**: `HTTP 204 No Content`

---
@anchor h_wifi_check_do
* Wi-Fi network availability test
    @code{text}
    POST /_/wifi/check.do
    @endcode

    Checking the availability of a Wi-Fi network using profile

    **Request Body**:
    @code{json}
    {
      "profile_index": 1
    }
    @endcode
    **Fields**:
    - `profile_index`: Profile number (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>@ref EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)

    **Response**: `HTTP 202 Accepted`

    @warning This operation can take quite a long time (tens of seconds). During the test, the ESP also disconnects from the actual Wi-Fi network and attempts to connect to the network being tested. This is done due to the physical limitations of the ESP
    

---
@anchor h_wifi_result_json
* Get the results of the Wi-Fi test
    @code{text}
    POST /_/wifi/result.json
    @endcode

    @note The use of `POST` is due to the fact that `Internet Explorer 8` does not support sending a `GET` request with a body.

    Get the results of the Wi-Fi network availability test

    **Request Body**:
    @code{json}
    {
      "profile_index": 1
    }
    @endcode
    **Fields**:
    - `profile_index`: Profile number (`0` - `X`, is set using `eif_set_wifi_profiles_count()`, otherwise it is <code>@ref EIF_WIFI_PROFILES_DEFAULT_COUNT</code>)

    **Response body**:
    @code{json}
    {
      "result": true,
      "rssi": -72
    }
    @endcode
    **Fields**:
    - `result`: `true` if a connection was established and an IP address was obtained, `false` otherwise
    - `rssi`: Signal strength in dBm

    @warning Due to the change of Wi-Fi networks for the test, this request may be sent too early and the server may not even see it. To get the result, use a loop of requests after some time has passed since the test started (about 8 seconds)


---

### TLS Management
@note Only available if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled.

@anchor h_tls_recreate_do
* Regenerate TLS keys and certificate
    @code{text}
    POST /_/tls/recreate.do
    @endcode

    Generates a new self-signed TLS certificate and keys

    **Request Body**: `No body`

    **Response**: `HTTP 202 Accepted`

    @warning After regenerating TLS keys and certificate, the device will automatically restart to apply the new configuration. The entire process usually takes less than 5 seconds.


---

### System Management

@anchor h_sys_info_json
* Get system information
    @code{text}
    GET /_/sys/info.json
    @endcode

    Returns detailed system information.
   
    **Request body**: `No body`

    **Response body**:
    @code{json}
    {
      "features": {
        "has_wifi": true,
        "has_bluetooth": true,
        "has_ble": true
      },
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
      "features": "WBL",
      "mac": "AA:BB:CC:DD:EE:FF"
    }
    @endcode
    **Fields**:
    - `features`: Object containing supported hardware features
      - `has_wifi`: `true` if Wi-Fi (802.11 b/g/n) is supported
      - `has_bluetooth`: `true` if Bluetooth Classic is supported
      - `has_ble`: `true` if Bluetooth Low Energy (BLE) is supported
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
    - `mac`: Device MAC address

---

@anchor h_sys_reboot_do
* Reboot System
    @code{text}
    POST /_/sys/reboot.do
    @endcode

    Gracefully reboot the device.

    **Request body**: `No body`

    **Response**: `HTTP 202 Accepted`

    @warning The device will reboot immediately after this request. All active connections will be closed.


---

### OTA Updates

@anchor h_ota_info_json
* Get firmware information
    @code{text}
    GET /_/ota/info.json
    @endcode

    Returns current firmware information.

    **Request body**: `No body`

    **Response body**:
    @code{json}
    {
      "project": "my_iot_project",
      "version": "1.0.0",
      "build_id": "a1b2c3d4e5f6...",
      "build_date": "Jan 1 2026",
      "build_time": "12:00:00",
      "idf_version": "v4.4.6",
      "compiler": "gcc 8.4.0",
      "target": "esp32",
      "partition": "factory",
      "ota_status": "valid"
    }
    @endcode
    **Fields**:
    - `project`: Project name
    - `version`: Firmware version
    - `build_id`: SHA256 of ELF file
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
@anchor h_ota_update_do
* Upload firmware
    @code{text}
    POST /_/ota/update.do
    @endcode

    Uploads a new firmware binary to the device. Once the upload is complete and verified, the device will automatically reboot to apply the update.

    **Request**:
    - Header `Content-Type`: `application/octet-stream`
    - Body: Raw firmware binary (`.bin` file)

    **Response**: `HTTP 202 Accepted`

    @warning The device will automatically restart after a successful upload. This process usually takes less than 5 seconds.


---
@anchor h_ota_confirm_do
* Confirm successful update
    @code{text}
    POST /_/ota/confirm.do
    @endcode

    Marks the current firmware as valid to prevent automatic rollback on the next boot.

    **Request body**: `No body`

    **Response**: `HTTP 202 Accepted`

---
@anchor h_ota_rollback_do
* Rollback firmware
    @code{text}
    POST /_/ota/rollback.do
    @endcode

    Rolls back to the previous firmware version. The device will automatically reboot.

    **Request body**: `No body`

    **Response**: `HTTP 202 Accepted`

    @warning The device will automatically restart immediately after the rollback is initiated. This process usually takes less than 5 seconds.


---

### Authentication Management
@note Only available if the Kconfig option `CONFIG_EIF_ENABLE_BASIC_AUTH` is enabled.

@anchor h_apass_update_do
* Update administrator password
    @code{text}
    POST /_/apass/update.do
    @endcode

    Changes the administrator password used for Basic Authentication.

    **Request body**:
    @code{json}
    {
      "password": "NewSecurePassword123"
    }
    @endcode
    **Fields**:
    - `password`: New password (<code>@ref EIF_BASIC_AUTH_PASS_MIN_LEN</code> - <code>@ref EIF_BASIC_AUTH_PASS_MAX_LEN - 1</code> characters)

    **Response**: `HTTP 204 No Content`

## Caching

### API Responses
- No caching for API endpoints
- `Cache-Control`: `no-store, no-cache, must-revalidate`
- `Expires`: `0`
- `Pragma`: `no-cache`

### Static Files
- `Cache-Control`: `public, max-age=<CONFIG_EIF_WEB_CACHE_MAX_AGE>`
- `ETag`: validation for cache revalidation