cmake_minimum_required(VERSION 3.0)
project(libvibrant VERSION 1.0.2 LANGUAGES C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)

add_definitions(-DVIBRANT_VERSION="${PROJECT_VERSION}")

add_subdirectory(vibrant)
add_subdirectory(vibrant-cli)
