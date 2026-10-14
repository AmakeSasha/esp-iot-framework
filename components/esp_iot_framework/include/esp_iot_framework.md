<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Path: ./components/esp_iot_framework_core/esp_iot_framework_core.md
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

<style>
  .md-alert { padding: 12px; margin: 16px 0; border-radius: 0 6px 6px 0; }
  .md-warn { background-color: #24211b; border-left: 4px solid #d29922; }
  .md-warn strong { color: #e3b341; }
  .md-note { background-color: #1f242c; border-left: 4px solid #388bfd; }
  .md-note strong { color: #58a6ff; }
</style>



# esp_iot_framework_core.h

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This group of modules is available when you include this line at the beginning of the file.:
    
    #include <esp_iot_framework_core.h>
</div>

The `CORE` serves as the essential foundation of the framework. It handles the critical low-level tasks—memory orchestration and system synchronization—to provide a reliable base for any application.

This module is designed to be universal: whether you are building a simple sensor node, a complex IoT device, or a multi-functional hub, the `CORE` remains the mandatory backbone. Even when using high-level libraries on top of it, the `CORE` continues to manage the system lifecycle and does not allow its essential functions to be overridden, ensuring consistent stability across all types of nodes.



---



## Entry

This module contains functions that must be called at the very beginning of the application to prepare internal data structures and system tasks.

### `eif_core_initialize`

```c
esp_err_t eif_core_initialize(void);
```

Sets up the internal configuration, prepares the web server settings, and launches essential system services.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called before any other <code>eif_*</code> functions (like URI registration or Wi-Fi connectivity). This function is <b>mandatory</b> for the framework to work.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    Calling this function overwrites any parameters previously set via <code>eif_*</code> API with default values.
</div>

**Returns**
- `ESP_OK`:         `CORE` initialization was successful.
- `ESP_ERR_NO_MEM`: This error **cannot be returned**, because the system automatically restarts due to the lack of a heap, even when the memory scanner is running.

Example of use:
```c
#include <esp_err.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    
    // Further code...
}
```

---

### `eif_nvs_initialize`

```c
esp_err_t eif_nvs_initialize(void);
```

This function performs the following essential steps:
- Initializes the NVS flash partition (with automatic repair/erase if corrupted).
- Prepares or loads [`Wi-Fi profiles`](#wifi_profiles_desc) based on the configured count.
- Manages TLS credentials and authentication data (if enabled).

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    When the device is started for the first time, this function will reboot the device, before that, TLS credentials will be created. This is done to reduce the RAM consumption of the device.
</div>

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a> but before <a href="#eif_wifi_initialize"><code>eif_wifi_initialize()</code></a>. This function is <b>mandatory</b> for the framework to work. If you use <a href="#eif_set_wifi_profiles_count"><code>eif_set_wifi_profiles_count()</code></a>, it must be called before calling <a href="#eif_nvs_initialize"><code>eif_nvs_initialize()</code></a>. Otherwise, the default number of profiles (<a href="#EIF_WIFI_PROFILES_DEFAULT_COUNT"><code>EIF_WIFI_PROFILES_DEFAULT_COUNT</code></a>) will be loaded into NVS.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    If the NVS partition is corrupted or has a new version, this function will automatically erase it and re-initialize, which results in the loss of previously stored data.
</div>

**Returns**
- `ESP_OK`:    NVS and all framework fields initialized successfully.
- `ESP_ERR_*`: Various NVS-related errors if hardware initialization fails. Look at the logs to understand the cause of the errors.

Example of use:
```c
#include <esp_err.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());

    // Further code...
}
```



---



## Event Handlers

These functions are the primary interface for linking the final product's business logic with system lifecycle events. They allow the application to execute specific tasks in direct response to internal state changes.

### `eif_handler_system_t`

```c
typedef esp_err_t (*eif_handler_system_t)(void);
```

Prototype for system-level lifecycle event handlers.

---

### `eif_register_handler_system_reboot`

```c
esp_err_t eif_register_handler_system_reboot(eif_handler_system_t handler);
```

This function hooks a custom handler into the system teardown process. It is the final safety net that triggers right before the system reset, allowing you to "park" your hardware or commit last-second logs.


<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    The handler runs within a dedicated reboot task with a limited stack size (<a href="../KCONFIG.md#CONFIG_EIF_REBOOT_TASK_STACK_SIZE"><code>CONFIG_EIF_REBOOT_TASK_STACK_SIZE()</code></a>. Avoid deep recursion or allocation of a large amount of memory (stack or heap).
</div>

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    Before this handler runs, the framework will automatically trigger the "IP Lost" (registered via <a href="./esp_iot_framework_core_ext.md#eif_register_handler_ip_lost"><code>eif_register_handler_ip_lost()</code></a>, ESP-IDF event: <code>IP_EVENT_STA_LOST_IP</code>) event and disable the Wi-Fi stack. This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a>.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    This registration is not cumulative. Any previously registered handler for this event will be overwritten. Only the most recent assignment is stored and executed by the <code>CORE</code>.
</div>

**Params**
- `handler`: Pointer to the function to execute. Cannot be `NULL`.

**Returns**
- `ESP_OK`:              Handler registered successfully.
- `ESP_ERR_INVALID_ARG`: The `handler` pointer is `NULL`.

Example of use:
```c
#include <esp_err.h>
#include <esp_iot_framework_core.h>

esp_err_t my_reboot_logic(void) {
    // Emergency state save or hardware shutdown
    return ESP_OK;
}

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_register_handler_system_reboot(my_reboot_logic));
   
    // Further code...
}
```



---



## Constants

This module contains default values and limits.

### Wi-Fi Configuration

<span id="EIF_WIFI_PROFILES_MAX_COUNT"></span>
```C
#define EIF_WIFI_PROFILES_MAX_COUNT 9U
```
Maximum number of custom [`Wi-Fi profiles`](#wifi_profiles_desc).

<span id="EIF_WIFI_PROFILES_DEFAULT_COUNT"></span>
```C
#define EIF_WIFI_PROFILES_DEFAULT_COUNT 2U
```
Default number of custom [`Wi-Fi profiles`](#wifi_profiles_desc).



---



## Wi-Fi Configuration

This module provides fine-grained control over the Wi-Fi stack behavior, including power management and connection recovery policies.

### `eif_set_wifi_config`
```C
esp_err_t eif_set_wifi_config(
    const wifi_init_config_t *wifi_driver_config,
    wifi_ps_type_t            wifi_power_mode,
    uint32_t                  wifi_attempt_delay_ms
);
```

Overrides the default low-level driver settings. These parameters are
applied globally and define the performance and energy profile of the device.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a> but before <a href="#eif_wifi_initialize"><code>eif_wifi_initialize()</code></a>.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    The function does not validate incoming data, which may result in unexpected behavior (<code>UB</code>), memory leaks, or Wi-Fi crashes. Always validate the data you send.
</div>

**Params**
- `wifi_driver_config`: Low-level ESP-IDF Wi-Fi driver configuration. Use `WIFI_INIT_CONFIG_DEFAULT()` (from `ESP-IDF`) as a base.
- `wifi_power_mode`: Wi-Fi Modem-sleep policy. Defines the trade-off between power consumption and network responsiveness/latency.
- `wifi_attempt_delay_ms`: Delay in milliseconds before switching to the next profile after a failed connection.

**Returns**
- `ESP_OK`:              Settings saved successfully.
- `ESP_ERR_INVALID_ARG`: The `wifi_driver_config` pointer is `NULL`.

Example of use:
```C
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());

    // Creating default Wi-Fi settings
    wifi_init_config_t cfg_wifi = WIFI_INIT_CONFIG_DEFAULT();
    // Changing Wi-Fi settings
    cfg_wifi.dynamic_rx_buf_num = 64;
    cfg_wifi.csi_enable = false;

    ESP_ERROR_CHECK(eif_set_wifi_config(&cfg_wifi, WIFI_PS_MIN_MODEM, 1000));

    // Further code...
}
```

---

### `eif_set_wifi_profiles_count`
```C
esp_err_t eif_set_wifi_profiles_count(uint8_t wifi_profiles_count);
```
Defines how many additional user-defined slots are available in NVS.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a> but before <a href="#eif_nvs_initialize"><code>eif_nvs_initialize()</code></a>.
</div>

<span id="wifi_profiles_desc"></span>
`Profile` - configuration for a specific Wi-Fi network (`SSID` and `Password`). Profiles are managed in a circular (ring) list: if a connection attempt fails or if the active connection is broken, the framework automatically rotates to the next available slot. Upon reaching the end of the list, it wraps around to the first profile. This cycle continues indefinitely until a stable connection is re-established.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    The total number of managed profiles is equal to (<code>wifi_profiles_count + 1</code>). If set to <code>0</code>, only the system default profile (index <code>0</code>) will be used. The default profile is hardcoded and cannot be changed. The default value is <a href="#EIF_WIFI_PROFILES_DEFAULT_COUNT"><code>EIF_WIFI_PROFILES_DEFAULT_COUNT</code></a>.
</div>

**Params**
- `wifi_profiles_count`: Number of extra slots (`0` to [`EIF_WIFI_PROFILES_MAX_COUNT`](#EIF_WIFI_PROFILES_MAX_COUNT)).

**Returns**
- `ESP_OK`:              Profile capacity set and state reset.
- `ESP_ERR_INVALID_ARG`: `wifi_profiles_count` exceeds [`EIF_WIFI_PROFILES_MAX_COUNT`](#EIF_WIFI_PROFILES_MAX_COUNT).

Example of use:
```C
#include <esp_err.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_set_wifi_profiles_count(5));
    ESP_ERROR_CHECK(eif_nvs_initialize());
    
    // Further code...
}
```

---

### `eif_wifi_initialize`
```C
esp_err_t eif_wifi_initialize(void);
```

Initializes the Wi-Fi stack, registers event handlers for automatic profile failover, and starts mDNS/Web Server once an IP is obtained. In case of IP loss, the Web Server and mDNS services are gracefully stopped to save resources and prevent invalid states. Once a new IP address is obtained, the framework automatically resumes these services.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function should come after any other <code>eif_*</code> functions (like URI registration or Wi-Fi connectivity). This function is <b>mandatory</b> for the framework to work.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    Otherwise, there is a chance that the settings will not be applied immediately or unexpected behavior (<code>UB</code>) will occur.
</div>

**Returns**
- `ESP_OK`:         Subsystem initialized successfully.
- `ESP_ERR_NO_MEM`: Failed to allocate memory for network interface.
- `ESP_ERR_*`:      Internal driver errors. Look at the logs to understand the cause of the errors.

Example of use:
```C
#include <esp_err.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_wifi_initialize());
   
    // Necessary to keep the flow alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```



---



## mDNS Configuration

This module allows you to configure mDNS. mDNS enables the device to be discovered via a human-readable name (e.g., `my-device.local`) instead of an IP address.

The framework automatically appends a unique identifier based on the MAC address to prevent network collisions (e.g., common hostname set: `esp32.local`, the framework turns this into `esp32-1a2b3c.local` on a device with MAC `XX:XX:XX:1A:2B:3C`).

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    In case the mDNS hostname is not set, <code>device</code> will be used as the common hostname.
</div>

### Constants

<span id="EIF_MDNS_TXT_RECORDS_MAX_COUNT"></span>
```C
#define EIF_MDNS_TXT_RECORDS_MAX_COUNT 32U
```
Maximum number of mDNS txt-records.

<span id="EIF_MDNS_HOSTNAME_MAX_LEN"></span>
```C
#define EIF_MDNS_HOSTNAME_MAX_LEN 57U
```
Maximum length of the mDNS hostname prefix including null-terminator (`\0`).

<span id="EIF_MDNS_INSTANCE_NAME_MAX_LEN"></span>
```C
#define EIF_MDNS_INSTANCE_NAME_MAX_LEN 64U
```
Maximum length of the mDNS instance name (friendly name) including null-terminator (`\0`).

---

### `eif_set_mdns`
```C
esp_err_t eif_set_mdns(
    const char * const hostname,
    const char * const instance_name
);
```
Sets the base identity of the device on the local network.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a> but before <a href="#eif_wifi_initialize"><code>eif_wifi_initialize()</code></a>.
</div>

**Params**
- `hostname`: Base name prefix (e.g., `my-sensor`). The length should be less than [`EIF_MDNS_HOSTNAME_MAX_LEN`](#EIF_MDNS_HOSTNAME_MAX_LEN). If empty, the default hostname will be used (`device`).
- `instance_name`: Friendly name for discovery tools (e.g., `Hall Sensor`). The length should be less than [`EIF_MDNS_INSTANCE_NAME_MAX_LEN`](#EIF_MDNS_INSTANCE_NAME_MAX_LEN). If empty, the formatted `mdns_hostname` (with MAC) will be used as the instance name by default. 

**Returns**
- `ESP_OK`:               Configuration stored.
- `ESP_ERR_INVALID_ARG`:  The pointer `hostname` or `instance_name` is `NULL`.
- `ESP_ERR_INVALID_SIZE`: The length of the `hostname` or `instance_name` is outside the acceptable range.

Example of use:
```C
#include <esp_err.h>
#include <esp_iot_framework_core.h>

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_mdns("my-esp", "Smart Controller"));

    // Further code...
}  
```

---

### `eif_set_mdns_records`
```C
esp_err_t eif_set_mdns_records(
    const mdns_txt_item_t txt_records[EIF_MDNS_TXT_RECORDS_MAX_COUNT],
    size_t txt_records_count
);
```

TXT records allow you to broadcast additional metadata about the device (e.g., firmware version, model, or status). The framework creates an internal copy of the structure, so the original data's lifecycle no longer matters after the call.

<div class="md-alert md-note">
    <strong>ℹ️ NOTE</strong><br>
    This function must be called after <a href="#eif_core_initialize"><code>eif_core_initialize()</code></a> but before <a href="#eif_wifi_initialize"><code>eif_wifi_initialize()</code></a>.
</div>

<div class="md-alert md-warn">
    <strong>⚠️ WARNING</strong><br>
    The handler array length must be equal to <code>txt_records_count</code>. If the count exceeds the actual array length, a <code>Buffer Overread</code> will occur. If the count is less than the actual length, error <code>ESP_ERR_INVALID_ARG</code> will return.<br><br>
    If the array is accessible by value at the function call site, you can use the following code to automatically calculate the length:
    
    sizeof(array) / sizeof(array[0])
</div>

**Params**
- `txt_records`: Array of mDNS TXT items. The length should be [`EIF_MDNS_TXT_RECORDS_MAX_COUNT`](#EIF_MDNS_TXT_RECORDS_MAX_COUNT).
- `txt_records_count`: Number of elements in the array.

**Returns**
- `ESP_OK`:               Records applied.
- `ESP_ERR_INVALID_ARG`:  The `txt_records_count` exceeds `EIF_MDNS_TXT_RECORDS_MAX_COUNT`, or the `txt_records` pointer is `NULL` when count is non-zero.
- `ESP_ERR_INVALID_SIZE`: The `txt_records` length is less than `txt_records_count`.

Example of use:
```C
#include <mdns.h>
#include <esp_err.h>
#include <esp_iot_framework_core.h>

static const mdns_txt_item_t my_txt_records[] = {
    {"friendly_name", "Kitchen Main Light"},
    {"room",          "Kitchen"},
    {"hardware",      "ESP32-S3-DevKit"},
    {"v",             "1.0.4"}
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_mdns_records(
        my_txt_records,
        sizeof(my_txt_records) / sizeof(my_txt_records[0])
    ));
    
    // Further code...
}
```