project(libvibrant C)

find_library(libdrm REQUIRED)
find_package(X11 REQUIRED)

add_library(vibrant SHARED src/vibrant.c src/ctm.c src/util.c src/utilx.c)

target_include_directories(vibrant PUBLIC include)
target_include_directories(vibrant PRIVATE src)

target_link_libraries(vibrant drm X11 Xrandr)

set_target_properties(vibrant PROPERTIES VERSION 1.0.0)
set_target_properties(vibrant PROPERTIES SOVERSION 1)

install(TARGETS vibrant DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
