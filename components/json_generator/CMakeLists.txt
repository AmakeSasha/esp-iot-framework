# SPDX-License-Identifier: Apache-2.0
# Library: json_generator
# Folder: ./components/json_generator
# File: CMakeLists.txt
# Relative folder: .
# Original Author: Piyush Shah
# Version: 1.2.0
# Source: https://github.com/espressif/json_generator
# 
# Copyright 2020 Piyush Shah <shahpiyushv@gmail.com>
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

idf_component_register(
    SRCS "src/json_generator.c"
    INCLUDE_DIRS "include")