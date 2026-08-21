<!--
  SPDX-License-Identifier: Apache-2.0
  Example: relay_control_server
  Folder: ./examples/relay_control_server
  File: REST_API.md
  
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

## Map URIs

* `/` - Main interface container
* `/api`
    * `/relay`
        * `/on.do` - [Turn on the relay](#h_api_relay_on)
        * `/off.do` - [Turn off the relay](#h_api_relay_off)
        * `/toggle.do` - [Toggle the relay state](#h_api_relay_toggle)
        * `/toggle_logic.do` - [Toggle the hardware logic mode of the relay (`INVERTED`/`DIRECT`)](#h_api_relay_toggle_logic)
        * `/status.json` - [Get the state of the relay](#h_api_relay_status)

The entire API is also available from the [SERVER](../../components/esp_iot_framework_server/REST_API.md) node.

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
* Switch the direction of the relay to DOWN
    ```text
    POST /api/relay/off.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_relay_toggle"></a>
* Stop the movement of the relay
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

    <div style="background-color: #fff3cd; border-left: 5px solid #ffc107; padding: 12px; margin: 10px 0; color: #856404;">
    <strong>Warning</strong><br>
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
      "ligoc_str": "INVERTED",
      "state_str": "PIN_OFF",
      "number_of_changes": 24
    }
    ```
    **Fields**:
    - `ligoc_str:` Hardware logic configuration type of the relay (`INVERTED`/`DIRECT`)
    - `state_str`: Current physical voltage level of the relay pin (`PIN_ON`/`PIN_OFF`)
    - `number_of_changes`: Total count of all switching operations performed since system boot