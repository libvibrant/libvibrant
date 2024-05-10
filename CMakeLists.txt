cmake_minimum_required(VERSION 3.16)

project(vibrant LANGUAGES C VERSION 1.1.1)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(VIBRANT_ENABLE_TESTS "Enable tests" OFF)

include(GNUInstallDirs)
include(CTest)

# Dependencies

find_package(X11 REQUIRED COMPONENTS Xrandr)
find_library(XNVCtrl_LIB XNVCtrl)
find_library(m_LIB m)

# Create lib

add_library(vibrant SHARED)
target_sources(vibrant PRIVATE src/vibrant.c src/ctm.c src/util.c src/nvidia.c)
target_sources(vibrant PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/vibrant/ctm.h include/vibrant/nvidia.h include/vibrant/vibrant.h
)

set_target_properties(vibrant PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})
set_target_properties(vibrant PROPERTIES SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})
target_compile_definitions(vibrant PUBLIC VIBRANT_VERSION="${CMAKE_PROJECT_VERSION}")

target_link_libraries(vibrant PUBLIC ${X11_LIBRARIES} ${X11_Xrandr_LIB} PRIVATE ${XNVCtrl_LIB} ${m_LIB})

# Install

install(TARGETS vibrant DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT lib)
configure_file(src/vibrant.pc.in src/vibrant.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/vibrant.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT lib)

# Tests

if (VIBRANT_ENABLE_TESTS)
    add_subdirectory(tests)
else ()
    message("Tests have been disabled. Set VIBRANT_ENABLE_TESTS=ON to enable them")
endif ()

# CLI

add_subdirectory(cli)
