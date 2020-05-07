cmake_minimum_required(VERSION 3.0)

project(vibrant-cli C)

find_package(X11 REQUIRED)

add_executable(vibrant-cli src/main.c)
target_link_libraries(vibrant-cli vibrant X11)

install(TARGETS vibrant-cli DESTINATION ${CMAKE_INSTALL_BINDIR})
