cmake_minimum_required(VERSION 3.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(saba)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/external/googletest/googletest/include)

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(gtests)
