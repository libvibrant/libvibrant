cmake_minimum_required(VERSION 3.0)
project(libvibrant VERSION 0.0.2 LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)

add_subdirectory(vibrant)
add_subdirectory(vibrant-cli)
