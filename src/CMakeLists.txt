
file (GLOB BASE_SOURCE Saba/Base/*.cpp)
file (GLOB BASE_HEADER Saba/Base/*.h)

file (GLOB GL_SOURCE Saba/GL/*.cpp)
file (GLOB GL_HEADER Saba/GL/*.h)
file (GLOB GL_MODEL_OBJ_SOURCE Saba/GL/Model/OBJ/*.cpp)
file (GLOB GL_MODEL_OBJ_HEADER Saba/GL/Model/OBJ/*.h)

file (GLOB VIEWER_SOURCE Saba/Viewer/*.cpp)
file (GLOB VIEWER_HEADER Saba/Viewer/*.h)

file (GLOB MODEL_OBJ_SOURCE Saba/Model/OBJ/*.cpp)
file (GLOB MODEL_OBJ_HEADER Saba/Model/OBJ/*.h)

file (GLOB GL3W_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/*.c)

file (GLOB IMGUI_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/*.cpp)

add_library(
    Saba
    ${BASE_SOURCE}
    ${BASE_HEADER}
    ${GL_SOURCE}
    ${GL_HEADER}
    ${GL_MODEL_OBJ_SOURCE}
    ${GL_MODEL_OBJ_HEADER}
    ${VIEWER_SOURCE}
    ${VIEWER_HEADER}
    ${MODEL_OBJ_SOURCE}
    ${MODEL_OBJ_HEADER}
    ${GL3W_SOURCE}
    ${IMGUI_SOURCE}
)
target_link_libraries(Saba glfw glslang OSDependent SPIRV OGLCompiler HLSL)

SOURCE_GROUP(Base FILES ${BASE_SOURCE} ${BASE_HEADER})
SOURCE_GROUP(GL FILES ${GL_SOURCE} ${GL_HEADER})
SOURCE_GROUP(GL\\Model\\OBJ FILES ${GL_MODEL_OBJ_SOURCE} ${GL_MODEL_OBJ_HEADER})
SOURCE_GROUP(Viewer FILES ${VIEWER_SOURCE} ${VIEWER_HEADER})
SOURCE_GROUP(Model\\OBJ FILES ${MODEL_OBJ_SOURCE} ${MODEL_OBJ_HEADER})
SOURCE_GROUP(GL3W FILES ${GL3W_SOURCE})
SOURCE_GROUP(IMGUI FILES ${IMGUI_SOURCE})

# SabaViewer

add_executable(SabaViewer main.cpp)
target_link_libraries(SabaViewer Saba ${OPENGL_LIBRARY})


add_custom_command(TARGET SabaViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Saba/Viewer/resource
    ${CMAKE_CURRENT_BINARY_DIR}/resource
)
