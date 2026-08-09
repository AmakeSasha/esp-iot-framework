# MISRA C-2012

## misra-c2012-21.3

The `esp-iot-framework` uses `FreeRTOS` `pvPortMalloc()` and `vPortFree()` instead of the standard library functions `malloc()` and `free()` from `<stdlib.h>`. The framework does not actually use the prohibited stdlib functions.

The FreeRTOS heap functions provide deterministic management of a pre-allocated heap with built-in thread safety and seamless integration with ESP-IDF (required for Wi-Fi, Bluetooth, and networking stacks). They support memory capability checks through the heap_caps API (PSRAM vs. internal DRAM), built-in overflow detection, and configurable error hooks — none of which are guaranteed by the standard `malloc()`.

Dynamic allocation is minimal and limited to: NVS strings, TLS certificate buffers, URI handler registration, and OTA update buffers. All allocations have size limits, are protected by NULL checks, and include recovery mechanisms.

The framework does not include `<stdlib.h>` in its source code. Symbol table analysis confirms that `malloc`/`free` symbols resolve to the FreeRTOS heap functions. The CI/CD pipeline ensures the absence of stdlib memory allocation functions usage.

`Cppcheck 2.21+` flags `pvPortMalloc()` as a `Rule 21.3` violation due to signature analysis limitations in the static analyzer, despite the framework actually complying with the intent of the rule.

## misra-c2012-17.3

Hidden dependencies in macro definitions. Cppcheck incorrectly flags macro calls as violations despite proper parenthesization and guard expressions. This is a known limitation of Cppcheck's macro analysis affecting hundreds of macro call sites across the framework. Macros are properly defined with parenthesized arguments and conditional guards, complying with the intent of the rule.

## misra-c2012-8.7

The `esp-iot-framework` uses a decoupled architecture where Core components export public APIs to separate Nodes and End Devices. Cppcheck 2.21+ incorrectly flags these functions due to scope limitations. Its analysis is confined to individual component units, leaving it unaware of cross-component linkage and downstream business logic. 

Restricting these symbols to internal linkage (`static`) would break the framework's modular design and prevent external layers from accessing the Core engine. All public API functions are properly declared in header files, complying with the intent of the rule.

## misra-c2012-2.5

The `esp-iot-framework` header files include complete macro sets and configuration constants for diverse hardware setups. A significant portion of these macros is isolated by preprocessor guards (e.g., `#if defined(CONFIG_EIF_ENABLE_WEB_ADMIN_GUI)`). When a flag is disabled, the enclosed macros (such as `RESP_TYPE_PNG`) are compiled out. When enabled, they are fully utilized. The static analyzer evaluates headers in isolation or prior to full preprocessing, falsely flagging these conditionally active macros as unused.

Removing these symbols to satisfy Rule 2.5 would strip the SDK of conditional features and public API definitions. Since this rule is Advisory, a global framework-wide deviation is justified. All macros are structurally sound and introduce no side effects, fully satisfying the safety intent of the rule.

# Cppcheck

## unusedFunction

The framework functions as an SDK provider for external `End Devices`. Cppcheck flags public API entry points as `unusedFunction` because their invocations exist outside the framework's own compilation scope. This suppression is mandatory to prevent the analyzer from false-positively demanding the removal of public symbols.

## missingIncludeSystem

The framework relies on the `ESP-IDF` build system (`CMake`) and toolchain cross-compilers (`Xtensa`/`RISC-V`) to resolve system and platform-specific headers (`<stdlib.h>`, `freertos/`). Cppcheck lacks native integration with the ESP-IDF environment paths, leading to false `missingIncludeSystem` flags. Compilation verification is fully strictly handled by the `GCC`/`Clang` compiler toolchain in the CI/CD pipeline.