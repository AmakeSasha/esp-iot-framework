<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Example: stepper_control
  Folder: ./examples/stepper_control
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
    * `/stepper`
        * `/dir`
            * `/up.do` - [Switch the direction of the stepper to UP](#h_api_stepper_up)
            * `/down.do` - [Switch the direction of the stepper to DOWN](#h_api_stepper_down)
            * `/stop.do` - [Stop the movement of the stepper](#h_api_stepper_stop)
        * `/power`
            * `/on.do` - [Turn on the power of the stepper](#h_api_stepper_on)
            * `/off.do` - [Turn off the power of the stepper](#h_api_stepper_off)
        * `/status.json` - [Get the state of the stepper](#h_api_stepper_status)

The entire API is also available from the [Device](../../components/esp_iot_framework_device/REST_API.md) node.

## API Endpoints

<a id="h_api_stepper_up"></a>
* Switch the direction of the stepper to UP
    ```text
    GET /api/stepper/dir/up.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_stepper_down"></a>
* Switch the direction of the stepper to DOWN
    ```text
    GET /api/stepper/dir/down.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_stepper_stop"></a>
* Stop the movement of the stepper
    ```text
    GET /api/stepper/dir/stop.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_stepper_on"></a>
* Turn on the power of the stepper
    ```text
    GET /api/stepper/power/on.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_stepper_off"></a>
* Turn off the power of the stepper
    ```text
    GET /api/stepper/power/off.do
    ```
    
    **Request body**: `No body`
    
    **Response**: `HTTP 204 No Content`

---

<a id="h_api_stepper_status"></a>
* Get the state of the stepper
    ```text
    GET /api/stepper/status.json
    ```
    
    **Request body**: `No body`
    
    **Response body**:
    ```json
    {
      "current_dir": "STOP",
      "is_powered": false,
      "step_counter": 15,
      "steps_to_move": 995
    }
    ```
    **Fields**:
    - `current_dir`: What is the stepper doing right now: `STOP`, `UP`, `DOWN`
    - `is_powered`: Power state on the driver (`SLEEP_PIN`)
    - `step_counter`: The number of steps taken since the start
    - `steps_to_move`: The number of steps left to complete the move (if `steps_to_move` = `0` - no limits, if `steps_to_move` is greater than `0` - rotate `steps_to_move` steps)
