cmake_minimum_required(VERSION 3.2)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(saba)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option (SABA_BULLET_ROOT "Bullet Root Directory" "")
option (SABA_ENABLE_TEST "Enable Google test." on)
option (SABA_ENABLE_GL_TEST "OpenGL test." off)
option (SABA_USE_GLSLANG "glsl Preprocessor : glslang lib" off)
option (SABA_INSTALL "Saba install." off)
option (SABA_GLFW_ROOT "GLFW Root Directory" "")
option (SABA_FORCE_GLFW_BUILD "Force glfw build." off)
option (SABA_ENABLE_EXAMPLE_VULKAN "Build vulakn's example." off)

set (BULLET_ROOT ${SABA_BULLET_ROOT})

find_package (OpenGL REQUIRED)
find_package (Bullet REQUIRED)

if (NOT SABA_FORCE_GLFW_BUILD)
    set (GLFW_ROOT ${SABA_GLFW_ROOT})
    find_package (GLFW)
endif ()

if (NOT GLFW_FOUND)
    set (SABA_FORCE_GLFW_BUILD on CACHE BOOL "" FORCE)
endif ()

if (SABA_FORCE_GLFW_BUILD)
    include_directories (${PROJECT_SOURCE_DIR}/external/glfw/include)
else ()
    include_directories (${GLFW_INCLUDE_DIR})
endif ()

if (SABA_ENABLE_TEST)
    enable_testing()
endif()

if (SABA_ENABLE_EXAMPLE_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/viewer)
include_directories(${PROJECT_SOURCE_DIR}/external/gl3w/include)
include_directories(${PROJECT_SOURCE_DIR}/external/gli/include)
include_directories(${PROJECT_SOURCE_DIR}/external/glm/include)
include_directories(${PROJECT_SOURCE_DIR}/external/stb/include)
include_directories(${PROJECT_SOURCE_DIR}/external/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyobjloader/include)
include_directories(${PROJECT_SOURCE_DIR}/external/imgui/include)
include_directories(${PROJECT_SOURCE_DIR}/external/json/include)
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/external/lua)
include_directories(${PROJECT_SOURCE_DIR}/external/sol2/include)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyxfileloader/include)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyddsloader/include)
include_directories(${BULLET_INCLUDE_DIRS})

if(WIN32)
    ADD_DEFINITIONS(-DUNICODE)
    ADD_DEFINITIONS(-D_UNICODE)
endif(WIN32)

if(MSVC)
    ADD_DEFINITIONS(/MP)
endif()

add_subdirectory(external)

add_subdirectory(src)
add_subdirectory(viewer)
add_subdirectory(gtests)

add_executable(saba_viewer saba_viewer.cpp)
set (saba_viewer_LIBRARIES SabaViewer)
if (UNIX)
    list (APPEND saba_viewer_LIBRARIES ${CMAKE_DL_LIBS})
endif ()
target_link_libraries(saba_viewer ${saba_viewer_LIBRARIES})

add_custom_command(TARGET saba_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/viewer/Saba/Viewer/resource
    ${CMAKE_CURRENT_BINARY_DIR}/resource
)

add_executable(mmd2obj mmd2obj.cpp)
target_link_libraries(mmd2obj Saba)

add_subdirectory(example)

# Install
if (SABA_INSTALL)
    install (DIRECTORY src/Saba DESTINATION include  FILES_MATCHING PATTERN "*.h")
    install (TARGETS saba_viewer RUNTIME DESTINATION bin)
    install (DIRECTORY viewer/Saba/Viewer/resource DESTINATION bin)
    install (TARGETS mmd2obj RUNTIME DESTINATION bin)
endif()
