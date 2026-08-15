<!--
  SPDX-License-Identifier: Apache-2.0
  Project: esp-iot-framework
  Folder: ./docs
  File: authors_notes.md
  
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

 * [CORE, include/esp_iot_framework_core_ext.c, Task Management]
      Create proper documentation

 * [CORE, include/esp_iot_framework_core_ext.c, Other]
      Create proper documentation

 * [Examples, stepper_control]
      Add support for 'steps_to_move' in the REST API and web interface
-->

# Author's notes

Here are the answers that I considered important, at least for me. I think it will be useful for me to read this in the future.

---

* **Has the project been officially audited, certified, or checked for MISRA C compliance?**

  No official audit or commercial certification has been conducted (I don't have the budget for this). However, the code is checked through the `cppcheck` static analyzer. This reduces risks, eliminates known patterns of undefined behavior, and improves overall code reliability, although it does not replace a full-fledged industrial certification.

---

* **Is the framework guaranteed to have no bugs, error, failure or vulnerabilities in the project?**
  
  As an industry rule, no software is entirely immune to bugs or edge-case errors, especially during periods of active development. Therefore, users are strongly advised to thoroughly test the framework in their specific hardware environment before use.

---

* **Why might documentation sometimes not match the code or contradict itself?**

  At the moment, I am developing this project entirely on my own. Managing everything solo means constantly switching between different tasks like coding, writing documentation, and fixing bugs. This can sometimes make it tricky to keep track of every minor detail. Since I don't review the entire codebase from top to bottom before every single commit, small inconsistencies can occasionally slip through. However, whenever I come across them while working on the code, I fix them right away. If you spot any of these gaps, please feel free to open an Issues, and I will make sure to fix them!

---

* **What is the purpose of this project? What is its future?**
 
  This project is being created as the main (perhaps the only) part of my portfolio. I develop it when I feel like it. Since I really liked this project myself, I will continue to develop it in the future.

---

* **Where did the name of this project come from?**

  I just didn't want to waste time overthinking an overly fancy or complicated name. `ESP IoT Framework` describes exactly what this project is and does, so I went with it. It is straightforward, clean, and tells you its purpose right away.

---

* **Why do all functions, constants, and macros start with `eif_`? What does `eif` mean?**

  I did this on purpose, following the exact same pattern as ESP-IDF where everything starts with `esp_`. It completely rules out naming conflicts so your code and third-party libraries won't accidentally clash.

  As for the meaning, `eif` is simply a short prefix for the project's full name: <code><b>E</b>SP <b>I</b>oT <b>F</b>ramework</code>.

---

* **Why do component names start with the full project name instead of the abbreviation?**

  Because the abbreviation `eif` means absolutely nothing to someone who doesn't know the acronym. While the short `eif_` prefix is great for keeping your code concise, components should clearly show what project they belong to without forcing anyone to guess.