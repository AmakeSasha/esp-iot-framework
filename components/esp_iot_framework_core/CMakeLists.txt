# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Folder: ./components/esp_iot_framework_core
# File: CMakeLists.txt
# Library: esp_iot_framework_core
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

set(requires_components
    mdns
    driver
    esp_wifi
    nvs_flash
    esp_timer
    app_update
)

if(CONFIG_EIF_CORE_ENABLE_TLS)
    list(APPEND requires_components mbedtls)
    message(STATUS "EIF-CORE: component 'mbedTLS' included")
else()
    message(STATUS "EIF-CORE: component 'mbedTLS' excluded")
endif()

set(COMPONENT_NAME esp_iot_framework_core)
idf_component_register(
    SRCS "src/nvs.c"
         "src/system.c"
         "src/config.c"
         "src/network.c"
         "src/tls_manager.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src" "private_include"
    PRIV_REQUIRES ${requires_components}
)

set(comp_target "__idf_${COMPONENT_NAME}")
target_compile_options(${comp_target} PRIVATE "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}=.")

if(TARGET ${comp_target})
    if(CONFIG_EIF_CORE_ENABLE_TLS)
        target_link_libraries(${comp_target} PRIVATE idf::mbedtls)
    else()
    endif()
    if(TARGET idf::espressif__mdns)
        target_link_libraries(${comp_target} PRIVATE idf::espressif__mdns)
        message(STATUS "Linking mDNS via espressif__mdns (Registry)")
    elseif(TARGET idf::mdns)
        target_link_libraries(${comp_target} PRIVATE idf::mdns)
        message(STATUS "Linking mDNS via idf::mdns (Built-in)")
    else()
        message(FATAL_ERROR "mDNS component enabled but not found!")
    endif()
endif()