<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp_iot_framework
  Folder: .
  File: README.md
  
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

<div align="center">
    <h1>esp_iot_framework <small><font color="gray" style="font-weight: normal;">v0.2.1</font></small></h1>
    <h3>A framework for building scalable IoT ecosystems, custom nodes, and smart end-devices</h3>
</div>

<h1>Core Architecture Concept</h1>

The framework is built around three fundamental concepts, decoupling the infrastructure from your business logic:

* <b>Core (<code>[esp_iot_framework_core](./components/esp_iot_framework_core/README.md)</code>)</b> - The ecosystem engine. It handles connectivity, security, and device resilience. Anything built on top of *Core* inherits these critical properties for free.

---

* **Nodes** - The abstraction of a network unit built on top of *Core*. A Node implements a specific interaction pattern with the outside world — whether it is a web interface, an industrial protocol, or a cloud bridge. It hides the complexity of network interaction and provides a clean layer for business logic development.

  Examples of nodes:
  * <b>Device (<code>[esp_iot_framework_device](./components/esp_iot_framework_device/README.md)</code>)</b> - A foundational network node based on Espressif chips, designed for building fully manageable end IoT products.

---

* **End Devices** - A Node augmented with your product's business logic. Smart bulbs, relays, sensors, actuators — any target scenario boils down to implementing that single, unique logic on top of the chosen Node.

<h1>Storage, documentation and mirrors</h1>

To protect against network restrictions, blockages, and infrastructure failures, the project source code and technical documentation are synchronized between independent platforms.

| Platform               | Links                                                                                                                                                                  |
|:-----------------------|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **SourceCraft** (main) | [🔗 **Source code**](https://sourcecraft.dev/amakesasha/esp-iot-framework) <br> [📄 **Documentation**](https://amakesasha.sourcecraft.site/esp-iot-framework/)         |
| **GitHub** (mirror)    | [🔗 **Source code**](https://github.com/AmakeSasha/esp-iot-framework) <br>      [📄 **Documentation**](https://amakesasha.github.io/esp-iot-framework/html/index.html) |
| **GitVerse** (mirror)  | [🔗 **Source code**](https://gitverse.ru/amakesasha/esp-iot-framework) <br>     **There is no documentation**                                                          |

<h1>License</h1>

This project is licensed under the [`Apache 2.0 License`](./LICENSE), with the following exceptions:

* Contents of the [`/examples`](./examples) directory, which are dedicated to the `Public Domain` (or licensed under [`CC0 1.0`](./examples/LICENSE_CC0_1_0), at your option).
* Contents of the `firmware` branch (see its own `README` for details).

Please also refer to the [`NOTICE`](./NOTICE) file.