<div align="center">
    <h1>esp_iot_framework <small><font color="gray" style="font-weight: normal;">v0.2.1</font></small></h1>
    <h3>A framework for building scalable IoT ecosystems, custom nodes, and smart end-devices</h3>
</div>

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

<h1>Storage, documentation and mirrors</h1>

To protect against network restrictions, blockages, and infrastructure failures, the project source code and technical documentation are synchronized between independent platforms.

| Platform               | Links                                                                                                                                                                  |
|------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **SourceCraft** (main) | [🔗 **Source code**](https://sourcecraft.dev/amakesasha/esp-iot-framework) <br> [📄 **Documentation**](https://amakesasha.sourcecraft.site/esp-iot-framework/)         |
| **GitHub** (mirror)    | [🔗 **Source code**](https://github.com/AmakeSasha/esp-iot-framework) <br>      [📄 **Documentation**](https://amakesasha.github.io/esp-iot-framework/html/index.html) |
| **GitVerse** (mirror)  | [🔗 **Source code**](https://gitverse.ru/amakesasha/esp-iot-framework) <br>     **There is no documentation**                                                          |

<h1>License</h1>

This project is licensed under the [`Apache 2.0 License`](./LICENSE), except for the contents of the [`/examples`](./examples) directory, which are dedicated to the `Public Domain` (or [`CC0 1.0`](./examples/LICENSE_CC0_1_0) licensed, at your option). Also look at file [`NOTICE`](./NOTICE).