project(vibrant-cli C)

add_executable(vibrant-cli src/main.c)
target_link_libraries(vibrant-cli vibrant)

install(TARGETS vibrant-cli DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cli OPTIONAL)
