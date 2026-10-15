# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Path: ./components/esp_iot_framework/CMakeLists.txt
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
    jsmn
    mdns
    driver
    esp_wifi
    spi_flash
    nvs_flash
    esp_timer
    app_update
    json_generator
)

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
    message(STATUS "EIF: component 'Web GUI' included")
    if(CONFIG_EIF_ENABLE_BASIC_AUTH)
        list(APPEND embed_files "web/e401.html.gz")
    endif()
    if(CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG)
        list(APPEND embed_files "web/logs.html.gz")
    endif()
else()
    set(embed_files "")
    message(STATUS "EIF: component 'Web GUI' excluded")
endif()

if(CONFIG_EIF_ENABLE_TLS)
    list(APPEND requires_components mbedtls esp_https_server esp_https_ota)
    message(STATUS "EIF: component 'mbedTLS', 'TLS' included, using 'HTTPS'")
else()
    list(APPEND requires_components esp_http_server)
    message(STATUS "EIF: component 'mbedTLS', 'TLS' excluded, using 'HTTP'")
endif()

set(COMPONENT_NAME esp_iot_framework)
idf_component_register(
    SRCS "src/nvs.c"
         "src/system.c"
         "src/config.c"
         "src/network.c"
         "src/tls_manager.c"
         "src/http_server.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES ${requires_components}
    EMBED_FILES ${embed_files}
)

set(comp_target "__idf_${COMPONENT_NAME}")
target_compile_options(${comp_target} PRIVATE "-fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}=."
                                              "-Wall" "-Wextra" "-Werror")

if(TARGET ${comp_target})
    if(CONFIG_EIF_ENABLE_TLS)
        target_link_libraries(${comp_target} PRIVATE idf::mbedtls idf::esp_https_server idf::esp_https_ota)
    else()
        target_link_libraries(${comp_target} PRIVATE idf::esp_http_server)
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