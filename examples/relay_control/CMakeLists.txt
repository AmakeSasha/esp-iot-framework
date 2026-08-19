# SPDX-License-Identifier: Apache-2.0
# Example: relay_control
# Folder: ./examples/relay_control
# File: CmakeLists.txt
# 
# Copyright 2026 AmakeSasha
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

cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "../../components/esp_iot_framework_device/sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(example_relay_control)