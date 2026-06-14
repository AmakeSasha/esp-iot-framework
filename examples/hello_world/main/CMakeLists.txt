idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_iot_framework_core esp_iot_framework_device esp_https_server esp_wifi esp_event)