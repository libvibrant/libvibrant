cmake_minimum_required(VERSION 3.0)
project(vibrantX C)

set(CMAKE_C_STANDARD 99)

find_library(libdrm REQUIRED)
find_package(X11 REQUIRED)

add_executable(vibrantX main.c)

target_link_libraries(vibrantX drm X11 Xrandr)

install(TARGETS vibrantX DESTINATION bin)
