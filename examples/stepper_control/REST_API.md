<!-- SPDX-License-Identifier: CC0-1.0 -->

# REST API

The `stepper_control` example exposes a set of endpoints for controlling the stepper motor and reading its current state. The framework's built-in administrative API is described in the [`esp_iot_framework`](../../docs/REST_API.md) documentation.

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

## HTTP Status Codes

* `200 OK` - Successful GET request
* `204 No Content` - Successful POST request
* `400 Bad Request` - Invalid request parameters

## API Endpoints

### Stepper Control

<a id="h_api_stepper_up"></a>
Switch the direction of the stepper to UP
```text
POST /api/stepper/dir/up.do
```

Starts moving the stepper upward. If the `X-Steps` header is omitted or set to `0`, the motor rotates infinitely until stopped.

**Request**:
* **Headers** (Optional):
  * `X-Steps`: `[uint32]` Number of steps to move. If omitted or set to `0`, the motor rotates infinitely.

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_stepper_down"></a>
Switch the direction of the stepper to DOWN
```text
POST /api/stepper/dir/down.do
```

Starts moving the stepper downward. If the `X-Steps` header is omitted or set to `0`, the motor rotates infinitely until stopped.

**Request**:
* **Headers** (Optional):
  * `X-Steps`: `[uint32]` Number of steps to move. If omitted or set to `0`, the motor rotates infinitely.

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_stepper_stop"></a>
Stop the movement of the stepper
```text
POST /api/stepper/dir/stop.do
```

Stops the stepper motion while keeping the current power state unchanged.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_stepper_on"></a>
Turn on the power of the stepper
```text
POST /api/stepper/power/on.do
```

Enables the driver power (`SLEEP_PIN` released), allowing the holding current to stabilize before the first steps.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_stepper_off"></a>
Turn off the power of the stepper
```text
POST /api/stepper/power/off.do
```

Disables the driver power (`SLEEP_PIN` asserted), releasing the shaft.

**Request**: `Nothing`

**Response**:
* `204`: **Successful execution**
* `400`: **Invalid request**

---

<a id="h_api_stepper_status"></a>
Get the state of the stepper
```text
GET /api/stepper/status.json
```

Returns the current stepper state and movement statistics.

**Request**: `Nothing`

**Response**:
* `200`: **Successful execution**
  ```json
  {
    "current_dir": "STOP",
    "is_powered": false,
    "step_counter": 15,
    "steps_to_move": 995
  }
  ```
  **OR**
  ```json
  {
    "current_dir": "STOP",
    "is_powered": false,
    "step_counter": 15
  }
  ```
  **Fields**:
  - `current_dir`: What the stepper is doing right now: `STOP`, `UP`, `DOWN`
  - `is_powered`: Power state on the driver (`SLEEP_PIN`)
  - `step_counter`: The number of steps taken since the start
  - `steps_to_move`: The number of steps left to complete the move. If there are no limits, the field is absent from the response.
