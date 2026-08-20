<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: .
  File: AUTHOR_NOTES.md
  
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

<!--
                        FOR THE FUTURE:
 * [Project]
      Add unity-based tests

 * [Components]
      Create a 'HUB' node

 * [Examples]
      Add example `relay_controler`

 * [CORE, include/esp_iot_framework_core_ext.c, Task Management]
      Create proper documentation
-->

<!--
                        HUB FEATURES:

  * Automatic discovery of ecosystem devices on the network
  * Storing a list of devices and their status
  * A system for periodically polling devices and their status
  * Forwarding commands to the end device
  * Automatic OTA device updates from a dedicated server
  * A scripting language interpreter for executing custom logic
-->

# Author's notes

Here are the answers that I considered important, at least for me. I think it will be useful for me to read this in the future.

---

## Reliability

* **Has the project been officially audited, certified, or checked for MISRA C compliance?**

  No official audit or commercial certification has been conducted (I don't have the budget for this). However, the code is checked through the `cppcheck` static analyzer. This reduces risks, eliminates known patterns of undefined behavior, and improves overall code reliability, although it does not replace a full-fledged industrial certification.

---

* **Is the framework guaranteed to have no bugs, error, failure or vulnerabilities in the project?**
  
  As an industry rule, no software is entirely immune to bugs or edge-case errors, especially during periods of active development. Therefore, users are strongly advised to thoroughly test the framework in their specific hardware environment before use.

---

* **Why might documentation sometimes not match the code or contradict itself?**

  At the moment, I am developing this project entirely on my own. Managing everything solo means constantly switching between different tasks like coding, writing documentation, and fixing bugs. This can sometimes make it tricky to keep track of every minor detail. Since I don't review the entire codebase from top to bottom before every single commit, small inconsistencies can occasionally slip through. However, whenever I come across them while working on the code, I fix them right away. If you spot any of these gaps, please feel free to open an Issues, and I will make sure to fix them!

---

## Motivation

* **How did this project start?**

  On `January 15, 2026`, I needed a way to remotely control a stepper motor. Since I already had a smart speaker, I initially looked for ready-made firmware or libraries to emulate devices from other ecosystems. However, I couldn't find anything that satisfied me, so I decided to write my own solution-right when I had just started learning `C`. I wrote what I now consider a very crude piece of code. My original plan was to create a simple repository with the code and soldering instructions for the control board. But I struggled with drawing the electrical schematic and even thought about dropping the idea of publishing the project online altogether, since it had already solved my personal problem. Then, on `March 3, 2026`, I realized that only 5% of the code was actually controlling the stepper motor (yes, it took me that long, but learning how to solder ate up a massive amount of time). The remaining 95% could be completely reused for other projects. That is exactly when I decided to turn it into this framework. Of course, after this realization, I heavily modernized the code, so you won't get to see the very first version `0.0.0` :)

---

* **What is the philosophy behind this project?**

  For the most part, the philosophy of this project was shaped by its history. Since there was absolutely no thought of a hub when writing version `0.0.0`, the following core principles were established:

  * `Separation of concerns` - The framework does not handle business logic or try to be a ready-made ecosystem. It completely offloads the routine infrastructure, allowing the developer to focus exclusively on writing the logic for the specific device without being distracted by low-level services.
  * `Direct interaction` - Interaction with the device must happen directly without unnecessary layers. Whether through a standard web browser, `curl`, or similar CLI tools, it eliminates the need to install specialized applications or maintain heavy software.
  * `Absolute autonomy without a hub` - The device must be 100% self-sufficient and fully operational without any internet access or the presence of a dedicated hub. In view of the `Direct interaction` principle, the `HTTP(S)` protocol is the best fit for this criterion. In this architecture, the end devices act as standalone servers, while a potential hub becomes merely one of many clients. This approach guarantees that even a single smart relay remains completely functional on its own.

---

* **What is the purpose of this project? What is its future?**
 
  This project is being created as the main (perhaps the only) part of my portfolio. I develop it when I feel like it. Since I really liked this project myself, I will continue to develop it in the future.

---

* **Where did the name of this project come from?**

  I just didn't want to waste time overthinking an overly fancy or complicated name. `ESP IoT Framework` describes exactly what this project is and does, so I went with it. It is straightforward, clean, and tells you its purpose right away.

---

## Architecture

* **Will this framework support other popular IoT protocols in the future?**

  No. All popular alternatives have been carefully evaluated and intentionally rejected because they completely contradict the core principles of this project:

  * `MQTT` - These require a dedicated central server (broker) to route messages between devices. This directly violates the `Absolute Autonomy Without a Hub` principle. The framework will not force the user to deploy and maintain heavy third-party infrastructure just to make a single device work.
  * `CoAP` - Although it shares a similar request-response model with `HTTP`, it runs over `UDP` and requires specialized gateways or proxy servers. Standard web browsers and common utilities like `curl` cannot interact with `CoAP` natively out of the box. This completely destroys the `Direct Interaction` principle by forcing the user to install additional software.
  * `WebSockets` - This protocol is completely redundant. A standard REST API approach is more than enough to handle any device control or data retrieval tasks implemented in the development of the business logic. Personally, I see no real-world scenarios within the scope of this framework where `WebSockets` would perform better than standard `HTTP(S)`.

  Standard, pure `HTTP(S)` is the only protocol that perfectly satisfies all requirements of this framework, allowing the end device to remain a truly self-sufficient server.

---

* **Why do all functions, constants, and macros start with `eif_`? What does `eif` mean?**

  I did this on purpose, following the exact same pattern as ESP-IDF where everything starts with `esp_`. It completely rules out naming conflicts so your code and third-party libraries won't accidentally clash.

  As for the meaning, `eif` is simply a short prefix for the project's full name: <code><b>E</b>SP <b>I</b>oT <b>F</b>ramework</code>.

---

* **Why do component names start with the full project name instead of the abbreviation?**

  Because the abbreviation `eif` means absolutely nothing to someone who doesn't know the acronym. While the short `eif_` prefix is great for keeping your code concise, components should clearly show what project they belong to without forcing anyone to guess.