project(vibrant LANGUAGES C)

find_library(libdrm REQUIRED)
find_package(X11 REQUIRED)

add_library(vibrant SHARED src/vibrant.c src/ctm.c src/util.c src/utilx.c src/nvidia.c include/vibrant/nvidia.h)

target_include_directories(vibrant PUBLIC include)
target_include_directories(vibrant PRIVATE src)

target_link_libraries(vibrant drm X11 Xrandr XNVCtrl)

set_target_properties(vibrant PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})
set_target_properties(vibrant PROPERTIES SOVERSION 0)

install(TARGETS vibrant DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
