# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Library: esp_iot_framework_device
# Folder: components/esp_iot_framework_device
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

if(CONFIG_EIF_ENABLE_WEB_ADMIN_GUI)
    set(embed_files
        "web/api.js.gz"
        "web/LICENSE.gz"
        "web/json2.js.gz"
        "web/e404.html.gz"
        "web/style.css.gz"
        "web/index.html.gz"
        "web/system.html.gz"
        "web/network.html.gz"
    )
    message(STATUS "EIF-DEVICE: component 'Web GUI' included")
    if(CONFIG_EIF_ENABLE_BASIC_AUTH)
        list(APPEND embed_files "web/e401.html.gz")
        message(STATUS "EIF-DEVICE: file 'e401.html' included")
    else()
        message(STATUS "EIF-DEVICE: file 'e401.html' excluded")
    endif()
else()
    set(embed_files "")
    message(STATUS "EIF-DEVICE: component 'Web GUI' excluded")
endif()

set(requires_components
    json
    esp_timer
    app_update
    esp_iot_framework_core 
)

set(code_files
    "src/web.c"
    "src/config.c"
)

if(CONFIG_EIF_ENABLE_TLS)
    list(APPEND requires_components esp_https_server esp_https_ota)
    message(STATUS "EIF-DEVICE: component 'TLS' included, using 'HTTPS'")
else()
    list(APPEND requires_components esp_http_server)
    message(STATUS "EIF-DEVICE: component 'TLS' excluded, using 'HTTP'")
endif()

if(CONFIG_EIF_ENABLE_MDNS)
    list(APPEND requires_components mdns)
    message(STATUS "EIF-DEVICE: component 'mDNS' included")
else()
    message(STATUS "EIF-DEVICE: component 'mDNS' excluded.")
endif()

set(COMPONENT_NAME esp_iot_framework_device)
idf_component_register(
    SRCS ${code_files}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src" "private_include"
    PRIV_REQUIRES ${requires_components}
    EMBED_FILES ${embed_files}
)

set(comp_target "__idf_${COMPONENT_NAME}")
target_compile_options(${comp_target} PRIVATE "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}=.")

if(TARGET ${comp_target})
    if(CONFIG_EIF_ENABLE_TLS)
        target_link_libraries(${comp_target} PRIVATE 
            idf::mbedtls idf::esp_https_server idf::esp_https_ota)
    else()
        target_link_libraries(${comp_target} PRIVATE idf::esp_http_server)
    endif()
    if(CONFIG_EIF_ENABLE_MDNS)
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
endif()