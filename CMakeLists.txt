cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(saba)

find_package(OpenGL REQUIRED)

enable_testing()
option(SABA_ENABLE_GL_TEST "OpenGL test." off)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/src/Saba)
include_directories(${PROJECT_SOURCE_DIR}/external/gl3w/include)

if(WIN32)
    ADD_DEFINITIONS(-DUNICODE)
    ADD_DEFINITIONS(-D_UNICODE)
endif(WIN32)

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(gtests)
