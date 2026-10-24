<!-- SPDX-License-Identifier: CC0-1.0 -->

# REST API

The `relay_control` example exposes a set of endpoints for controlling the relay and reading its current state. The framework's built-in administrative API is described in the [`esp_iot_framework`](../../docs/REST_API.md) documentation.

## Map URIs

* `/` - Main interface container
* `/api`
  * `/relay`
    * `/on.do` - [Turn on the relay](#h_api_relay_on)
    * `/off.do` - [Turn off the relay](#h_api_relay_off)
    * `/toggle.do` - [Toggle the relay state](#h_api_relay_toggle)
    * `/toggle_logic.do` - [Toggle the hardware logic mode of the relay (`INVERTED`/`DIRECT`)](#h_api_relay_toggle_logic)
    * `/status.json` - [Get the state of the relay](#h_api_relay_status)

## HTTP Status Codes

* `200 OK` - Successful GET request
* `204 No Content` - Successful POST
* `400 Bad Request` - Invalid request parameters

## API Endpoints

### Relay Control

<a id="h_api_relay_on"></a>
Turn on the relay
```text
POST /api/relay/on.do
```

Turns the relay pin to the `PIN_ON` state.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_relay_off"></a>
Turn off the relay
```text
POST /api/relay/off.do
```

Turns the relay pin to the `PIN_OFF` state.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_relay_toggle"></a>
Toggle the relay state
```text
POST /api/relay/toggle.do
```

Switches the relay to the opposite logical state.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_relay_toggle_logic"></a>
Toggle the hardware logic mode of the relay (`INVERTED`/`DIRECT`)
```text
POST /api/relay/toggle_logic.do
```

Switches the relay hardware logic interpretation mode used for the active relay module type (low-level or high-level trigger).

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

> ⚠️ WARNING
>
> Do not toggle this logic state unless absolutely necessary. Changing the logic mode does not physically update the current pin voltage level immediately, causing a temporary state mismatch between `/api/relay/status.json` and the actual relay hardware position.

---

<a id="h_api_relay_status"></a>
Get the state of the relay
```text
GET /api/relay/status.json
```

Returns the current relay state and statistics.

**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
  ```json
  {
    "logic_str": "INVERTED",
    "state_str": "PIN_OFF",
    "number_of_changes": 24
  }
  ```
  **Fields**:
  - `logic_str`: Hardware logic configuration type of the relay (`INVERTED`/`DIRECT`)
  - `state_str`: Current physical voltage level of the relay pin (`PIN_ON`/`PIN_OFF`)
  - `number_of_changes`: Total count of all switching operations performed since system boot
