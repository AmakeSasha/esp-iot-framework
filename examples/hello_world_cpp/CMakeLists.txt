cmake_minimum_required(VERSION 3.16)

set(SDKCONFIG_DEFAULTS "../../sdkconfig.defaults")
set(EXTRA_COMPONENT_DIRS "../../")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world_cpp_test)
