include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package(Check REQUIRED)

include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(check_util check_util.c)
target_link_libraries(check_util vibrant ${CHECK_LIBRARIES})
add_test(check_util ${TEST_PATH}/check_util)
