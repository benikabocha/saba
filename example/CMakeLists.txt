# Example

# simple_mmd_viewer_glfw
add_executable(simple_mmd_viewer_glfw
    simple_mmd_viewer_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/gl3w.c
)

set(simple_mmd_viewer_glfw_LIBRARIES
    ${OPENGL_LIBRARY}
    Saba
)
if(SABA_FORCE_GLFW_BUILD)
    list(APPEND simple_mmd_viewer_glfw_LIBRARIES glfw)
else()
    list(APPEND simple_mmd_viewer_glfw_LIBRARIES ${GLFW_LIBRARIES})
endif()
if(UNIX)
    list(APPEND simple_mmd_viewer_glfw_LIBRARIES ${CMAKE_DL_LIBS})
endif()
target_link_libraries(simple_mmd_viewer_glfw ${simple_mmd_viewer_glfw_LIBRARIES})

add_custom_command(TARGET simple_mmd_viewer_glfw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../viewer/Saba/Viewer/resource
    $<TARGET_FILE_DIR:simple_mmd_viewer_glfw>/resource
)

# simple_mmd_viewer_dx11
if(WIN32)
    add_executable(simple_mmd_viewer_dx11
        simple_mmd_viewer_dx11.cpp
    )
    target_link_libraries(simple_mmd_viewer_dx11
        D3D11
        Saba
    )
endif()

# simple_mmd_viewer_vulkan
if(SABA_ENABLE_EXAMPLE_VULKAN)
    include_directories(${Vulkan_INCLUDE_DIR})
    add_executable(simple_mmd_viewer_vulkan
        simple_mmd_viewer_vulkan.cpp
    )
    target_link_libraries(simple_mmd_viewer_vulkan
        ${Vulkan_LIBRARY}
        ${simple_mmd_viewer_glfw_LIBRARIES}
        Saba
    )
endif()
