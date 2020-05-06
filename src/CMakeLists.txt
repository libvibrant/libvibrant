cmake_minimum_required(VERSION 3.0)
project(vibrantX C)


find_package(X11 REQUIRED)

add_executable(vibrantX main.c)

target_link_libraries(vibrantX vibrant drm X11 Xrandr)

install(TARGETS vibrantX DESTINATION bin)
