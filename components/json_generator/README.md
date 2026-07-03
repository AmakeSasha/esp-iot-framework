<!--
  SPDX-License-Identifier: Apache-2.0
  Original Author: Piyush Shah
  Library: json_generator
  Version: 1.2.0
  Source: https://github.com/espressif/json_generator
  Folder: .
  File: README.md
  
  MIT License
  
  Copyright (c) 2010 Serge Zaitsev
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
-->

# JSON Generator

[![Component Registry](https://components.espressif.com/components/espressif/json_generator/badge.svg)](https://components.espressif.com/components/espressif/json_generator)

A simple JSON (JavasScript Object Notation) generator with flushing capability.
Details of JSON can be found at [http://www.json.org/](http://www.json.org/).
The JSON strings generated can be validated using any standard JSON validator. Eg. [https://jsonlint.com/](https://jsonlint.com/)

# Files
- `src/json_generator.c`: Actual source file for the JSON generator with implementation of all APIS
- `include/json_generator.h`: Header file documenting and exposing all available APIs

# Usage

Include the C and H files in your project's build system and that should be enough.
`json_generator` requires only standard library functions for compilation
