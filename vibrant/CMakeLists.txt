project(vibrant LANGUAGES C)

find_package(X11 REQUIRED)

add_library(vibrant SHARED src/vibrant.c src/ctm.c src/util.c src/nvidia.c)

target_include_directories(vibrant PUBLIC include)
target_include_directories(vibrant PRIVATE src)

target_link_libraries(vibrant X11 Xrandr XNVCtrl)

set_target_properties(vibrant PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})
set_target_properties(vibrant PROPERTIES SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})

install(TARGETS vibrant DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT lib)
