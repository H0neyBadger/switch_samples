
cmake_minimum_required(VERSION 3.2)

project(test)

# load switch.cmake toolchain form ./cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(switch)

# to compile plutonium third party
set(CMAKE_CXX_STANDARD 17)

add_executable(test WIN32
    main.cpp
)

target_link_libraries(test drm_nouveau nx m)

add_nro_target(test "cmake" "template" "0.0.1" "${CMAKE_SOURCE_DIR}/icon.jpg" "${CMAKE_SOURCE_DIR}/romfs")
