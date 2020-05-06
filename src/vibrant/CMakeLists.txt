cmake_minimum_required(VERSION 3.2)
project(vibrant C)

find_library(libdrm REQUIRED)
find_package(X11 REQUIRED)

add_library(vibrant SHARED vibrant.c)

install(TARGETS vibrant DESTINATION lib)
install(FILES vibrant.h ctm.h DESTINATION include)
