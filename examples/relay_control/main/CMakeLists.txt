# SPDX-License-Identifier: CC0-1.0

idf_component_register(
    SRCS "main.c" "relay.c"
    INCLUDE_DIRS "."
    EMBED_FILES "../web_src/index.html"
    REQUIRES esp_iot_framework esp_https_server esp_wifi esp_event mdns json_generator)