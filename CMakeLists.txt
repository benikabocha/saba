cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(saba)

option (SABA_BULLET_ROOT "Bullet Root Directory" "")
set (BULLET_ROOT ${SABA_BULLET_ROOT})

find_package(OpenGL REQUIRED)
find_package(Bullet REQUIRED)

enable_testing()
option(SABA_ENABLE_GL_TEST "OpenGL test." off)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/external/gl3w/include)
include_directories(${PROJECT_SOURCE_DIR}/external/gli/include)
include_directories(${PROJECT_SOURCE_DIR}/external/glm/include)
include_directories(${PROJECT_SOURCE_DIR}/external/stb/include)
include_directories(${PROJECT_SOURCE_DIR}/external/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/external/glslang)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyobjloader/include)
include_directories(${PROJECT_SOURCE_DIR}/external/imgui/include)
include_directories(${BULLET_INCLUDE_DIRS})

if(WIN32)
    ADD_DEFINITIONS(-DUNICODE)
    ADD_DEFINITIONS(-D_UNICODE)
endif(WIN32)

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(gtests)
