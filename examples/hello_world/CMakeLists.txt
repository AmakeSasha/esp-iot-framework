cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "../../components/esp_iot_framework_device/sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world_test)