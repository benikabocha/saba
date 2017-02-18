cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(saba)

option (SABA_BULLET_ROOT "Bullet Root Directory" "")
option (SABA_ENABLE_TEST "Enable Google test." on)
option (SABA_ENABLE_GL_TEST "OpenGL test." off)
option (SABA_USE_GLSLANG "glsl Preprocessor : glslang lib" off)

set (BULLET_ROOT ${SABA_BULLET_ROOT})

find_package(OpenGL REQUIRED)
find_package(Bullet REQUIRED)

if (SABA_ENABLE_TEST)
    enable_testing()
endif()

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/viewer)
include_directories(${PROJECT_SOURCE_DIR}/external/gl3w/include)
include_directories(${PROJECT_SOURCE_DIR}/external/gli/include)
include_directories(${PROJECT_SOURCE_DIR}/external/glm/include)
include_directories(${PROJECT_SOURCE_DIR}/external/stb/include)
include_directories(${PROJECT_SOURCE_DIR}/external/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/external/glslang)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyobjloader/include)
include_directories(${PROJECT_SOURCE_DIR}/external/imgui/include)
include_directories(${PROJECT_SOURCE_DIR}/external/picojson/include)
include_directories(${BULLET_INCLUDE_DIRS})

if(WIN32)
    ADD_DEFINITIONS(-DUNICODE)
    ADD_DEFINITIONS(-D_UNICODE)
endif(WIN32)

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(viewer)
add_subdirectory(gtests)

add_executable(saba_viewer saba_viewer.cpp)
target_link_libraries(saba_viewer SabaViewer)

add_custom_command(TARGET saba_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/Saba/Viewer/resource
    ${CMAKE_CURRENT_BINARY_DIR}/resource
)

add_executable(mmd2obj mmd2obj.cpp)
target_link_libraries(mmd2obj Saba)
