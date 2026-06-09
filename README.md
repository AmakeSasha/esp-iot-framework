<div align="center">
    <h1>esp_iot_framework</h1>
    <h3>A framework for creating reliable IoT devices based on the Espressif Systems devices</h3>
</div>

[![Origin SourceCraft](docs/badges/origin_sourcecraft.svg)](https://sourcecraft.dev/amakesasha/esp-iot-framework)
[![Mirror GitHub](docs/badges/mirror_github.svg)](https://github.com/amakesasha/esp-iot-framework)
[![Mirror GitVerse](docs/badges/mirror_gitverse.svg)](https://gitverse.ru/amakesasha/esp-iot-framework)

[![License](docs/badges/license.svg)](./LICENSE)
![Version](docs/badges/version.svg)
[![Documentation](docs/badges/documentation.svg)](https://amakesasha.sourcecraft.site/esp-iot-framework/)

<h1>Usage</h1>

To start using this framework, you must apply the settings from `sdkconfig.defaults` and connect it to your project. Configure your project's main `CMakeLists.txt` like this (replace `<TEXT>` with the desired values, without the `<>`s themselves):
```CMake
cmake_minimum_required(VERSION 3.16)

list(APPEND SDKCONFIG_DEFAULTS "<PATH_TO_FRAMEWORK>/components/esp_iot_framework_device/sdkconfig.defaults")
# If you are using your own FILE, add this line:
# list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults")
list(APPEND EXTRA_COMPONENT_DIRS "<PATH_TO_FRAMEWORK>/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<NAME_PROJECT>)
```

<h1>Documentation</h1>

See the site: https://amakesasha.sourcecraft.site/esp-iot-framework/

<h1>License</h1>

This project is licensed under the [`Apache 2.0 License`](./LICENSE), except for the contents of the [`/examples`](./examples) directory, which are dedicated to the `Public Domain` (or [`CC0 1.0`](./examples/LICENSE_CC0_1_0) licensed, at your option). Also look at file [`NOTICE`](./NOTICE).