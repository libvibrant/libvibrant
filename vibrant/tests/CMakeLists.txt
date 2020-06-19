find_package(Check REQUIRED)
find_package(Subunit)

include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(check_util check_util.c)
target_link_libraries(check_util vibrant ${CHECK_LIBRARIES})

if(SUBUNIT_FOUND)
    target_link_libraries(check_skilobot subunit)
endif()

add_test(check_util check_util)
