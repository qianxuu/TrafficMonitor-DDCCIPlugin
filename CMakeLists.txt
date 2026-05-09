cmake_minimum_required(VERSION 3.20)
project(DDCCIPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
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