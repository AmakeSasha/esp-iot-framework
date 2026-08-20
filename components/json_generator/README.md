<!--
  SPDX-License-Identifier: Apache-2.0
  Library: json_generator
  Folder: ./components/json_generator
  File: README.md
  Relative folder: .
  Original Author: Piyush Shah
  Version: 1.2.0
  Source: https://github.com/shahpiyushv/json_generator
  
  Copyright 2020 Piyush Shah <shahpiyushv@gmail.com>
  
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

  JSON Generator

[![Component Registry](https://components.espressif.com/components/espressif/json_generator/badge.svg)](https://components.espressif.com/components/espressif/json_generator)

A simple JSON (JavasScript Object Notation) generator with flushing capability.
Details of JSON can be found at [http://www.json.org/](http://www.json.org/).
The JSON strings generated can be validated using any standard JSON validator. Eg. [https://jsonlint.com/](https://jsonlint.com/)

  Files
- `src/json_generator.c`: Actual source file for the JSON generator with implementation of all APIS
- `include/json_generator.h`: Header file documenting and exposing all available APIs

  Usage

Include the C and H files in your project's build system and that should be enough.
`json_generator` requires only standard library functions for compilation
