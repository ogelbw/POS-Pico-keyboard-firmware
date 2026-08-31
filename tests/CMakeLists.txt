# Standalone host build of the unit tests. Does NOT use the Pico SDK.
#
#   cmake -S tests -B build/tests
#   cmake --build build/tests
#   ./build/tests/pico_keyboard_tests
cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(pico_keyboard_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pico_keyboard_tests
        test_main.cpp
        test_keymap.cpp
        test_pacing.cpp
        test_report_builder.cpp
        ../kb/keymap.cpp
        ../kb/pacing.cpp
        ../kb/report_builder.cpp
        )

# Project root, so the sources can #include "kb/...".
target_include_directories(pico_keyboard_tests PRIVATE ..)