idf_component_register(
    SRCS "main.c" "stepper.c"
    INCLUDE_DIRS "."
    EMBED_FILES "../web_src/index.html"
    REQUIRES esp_iot_framework_core esp_iot_framework_device esp_https_server esp_wifi esp_event)