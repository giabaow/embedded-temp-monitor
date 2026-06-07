cmake_minimum_required(VERSION 3.16)
project(embedded-temp-monitor LANGUAGES CXX)

# ── C++17 required for std::optional ────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler warnings (treat as errors in CI) ────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Shared library of core modules ───────────────────────────────────────────
add_library(monitor_core
    src/adc_hal.cpp
    src/temp_sensor.cpp
    src/alarm_manager.cpp
)
target_include_directories(monitor_core PUBLIC include)

# ── Main simulation executable ───────────────────────────────────────────────
add_executable(temp_monitor src/main.cpp)
target_link_libraries(temp_monitor PRIVATE monitor_core)

# ── Unit test executable ─────────────────────────────────────────────────────
add_executable(run_tests tests/test_main.cpp)
target_link_libraries(run_tests PRIVATE monitor_core)

# Register with CTest so `ctest` or `cmake --build . --target test` works
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)

# ── Cross-compilation note ───────────────────────────────────────────────────
# To target STM32 (Cortex-M4), invoke cmake with a toolchain file:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/stm32-toolchain.cmake ..
# The toolchain file sets CMAKE_C_COMPILER / CMAKE_CXX_COMPILER to
# arm-none-eabi-gcc/g++ and adds -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 flags.
