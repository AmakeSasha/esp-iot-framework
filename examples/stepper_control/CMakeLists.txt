# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "../../components/esp_iot_framework/sdkconfig.defaults"
                               "../sdkconfig.examples")
list(APPEND EXTRA_COMPONENT_DIRS "../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(example_stepper_control)