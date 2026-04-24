# SPDX-License-Identifier: Apache-2.0
# Project: esp_iot_framework
# Folder: .
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
    message(STATUS "Kernel component 'Web GUI' included")
    if(CONFIG_EIF_ENABLE_BASIC_AUTH)
        list(APPEND embed_files "web/e401.html.gz")
        message(STATUS "Kernel file 'e401.html' included")
    else()
        message(STATUS "Kernel file 'e401.html' excluded")
    endif()
else()
    set(embed_files "")
    message(STATUS "Kernel component 'Web GUI' excluded")
endif()

set(requires_components
    json
    esp_wifi
    nvs_flash
    app_update
)

set(code_files
    "src/nvs.c"
    "src/web.c"
    "src/system.c"
    "src/config.c"
    "src/network.c"
)

if(CONFIG_EIF_ENABLE_TLS)
    list(APPEND requires_components mbedtls esp_https_server esp_https_ota)
    list(APPEND code_files "src/cert_manager.c")
    message(STATUS "Kernel component 'mbedtls' included, using 'HTTPS'")
else()
    list(APPEND requires_components esp_http_server)
    message(STATUS "Kernel component 'mbedtls' excluded, using 'HTTP'")
endif()

if(CONFIG_EIF_ENABLE_MDNS)
    list(APPEND requires_components mdns)
    message(STATUS "Kernel component 'mDNS' included")
else()
    message(STATUS "Kernel component 'mDNS' excluded.")
endif()

set(COMPONENT_NAME esp-iot-framework)
idf_component_register(
    SRCS ${code_files}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src" "private_include"
    PRIV_REQUIRES ${requires_components}
    EMBED_FILES ${embed_files}
)

set(comp_target "__idf_${COMPONENT_NAME}")
if(TARGET ${comp_target})
    if(CONFIG_EIF_ENABLE_TLS)
        target_link_libraries(${comp_target} PRIVATE 
            idf::mbedtls idf::esp_https_server idf::esp_https_ota)
    else()
        target_link_libraries(${comp_target} PRIVATE idf::esp_http_server)
    endif()
    if(CONFIG_EIF_ENABLE_MDNS)
        target_link_libraries(${comp_target} PRIVATE idf::mdns)
    endif()
endif()