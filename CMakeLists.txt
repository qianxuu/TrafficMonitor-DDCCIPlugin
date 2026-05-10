cmake_minimum_required(VERSION 3.20)
project(DDCCIPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/utf-8)

add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
    src/MonitorController.cpp
)

target_include_directories(DDCCIPlugin PRIVATE
    include
    src
)

target_link_libraries(DDCCIPlugin PRIVATE
    dxva2
    user32
)

set_target_properties(DDCCIPlugin PROPERTIES
    LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/Plugin.def"
)

enable_testing()
add_executable(test_brightness_item
    test/test_brightness_item.cpp
    src/BrightnessItem.cpp
)
target_include_directories(test_brightness_item PRIVATE include src)
add_test(NAME test_brightness_item COMMAND test_brightness_item)

add_executable(test_plugin
    test/test_plugin.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
)
target_include_directories(test_plugin PRIVATE include src)
add_test(NAME test_plugin COMMAND test_plugin)
