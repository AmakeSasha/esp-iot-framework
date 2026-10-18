<!--  SPDX-License-Identifier: CC0-1.0 -->

<style>
  .md-alert { padding: 12px; margin: 16px 0; border-radius: 0 6px 6px 0; }
  .md-warn { background-color: #24211b; border-left: 4px solid #d29922; }
  .md-warn strong { color: #e3b341; }
  .md-note { background-color: #1f242c; border-left: 4px solid #388bfd; }
  .md-note strong { color: #58a6ff; }
</style>



## Map URIs

* `/` - Main interface container
* `/api`
    * `/relay`
        * `/on.do` - [Turn on the relay](#h_api_relay_on)
        * `/off.do` - [Turn off the relay](#h_api_relay_off)
        * `/toggle.do` - [Toggle the relay state](#h_api_relay_toggle)
        * `/toggle_logic.do` - [Toggle the hardware logic mode of the relay (`INVERTED`/`DIRECT`)](#h_api_relay_toggle_logic)
        * `/status.json` - [Get the state of the relay](#h_api_relay_status)

The entire API is also available from the [`esp_iot_framework`](../../components/esp_iot_framework/REST_API.md) node.

## HTTP Status Codes

- `200 OK` - Successful GET request
- `204 No Content` - Successful POST
- `400 Bad Request` - Invalid request parameters

## API Endpoints

<a id="h_api_relay_on"></a>
* Turn on the relay
    ```text
    POST /api/relay/on.do
    ```
    
    **Request body**: `No body`

    **Response**: `HTTP 204 No Content`

---

<a id="h_api_relay_off"></a>
* Turn off the relay
    ```text
    POST /api/relay/off.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_relay_toggle"></a>
* Toggle the relay
    ```text
    POST /api/relay/toggle.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_relay_toggle_logic"></a>
* Toggle the hardware logic mode of the relay (`INVERTED`/`DIRECT`)
    ```text
    POST /api/relay/toggle_logic.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

    <div class="md-alert md-warn">
        <strong>⚠️ WARNING</strong><br>
        Do not toggle this logic state unless absolutely necessary. Changing the logic mode does not physically update the current pin voltage level immediately, causing a temporary state mismatch between <code>/api/relay/status.json</code> and the actual relay hardware position.
    </div>

---

<a id="h_api_relay_status"></a>
* Get the state of the relay
    ```text
    GET /api/relay/status.json
    ```
    
    **Request body**: `No body`
    
    **Response body**:
    ```json
    {
      "logic_str": "INVERTED",
      "state_str": "PIN_OFF",
      "number_of_changes": 24
    }
    ```
    **Fields**:
    - `ligoc_str:` Hardware logic configuration type of the relay (`INVERTED`/`DIRECT`)
    - `state_str`: Current physical voltage level of the relay pin (`PIN_ON`/`PIN_OFF`)
    - `number_of_changes`: Total count of all switching operations performed since system boot