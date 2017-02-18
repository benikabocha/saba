set (
    GL_SOURCE
    Saba/GL/GLShaderUtil.cpp
    Saba/GL/GLSLUtil.cpp
    Saba/GL/GLTextureUtil.cpp
)
set (
    GL_HEADER
    Saba/GL/GLObject.h
    Saba/GL/GLShaderUtil.h
    Saba/GL/GLSLUtil.h
    Saba/GL/GLTextureUtil.h
    Saba/GL/GLVertexUtil.h
)
set (
    GL_MODEL_OBJ_SOURCE
    Saba/GL/Model/OBJ/GLOBJModel.cpp
    Saba/GL/Model/OBJ/GLOBJModelDrawContext.cpp
    Saba/GL/Model/OBJ/GLOBJModelDrawer.cpp
)
set (
    GL_MODEL_OBJ_HEADER
    Saba/GL/Model/OBJ/GLOBJModel.h
    Saba/GL/Model/OBJ/GLOBJModelDrawContext.h
    Saba/GL/Model/OBJ/GLOBJModelDrawer.h
)
set (
    GL_MODEL_MMD_SOURCE
    Saba/GL/Model/MMD/GLMMDModel.cpp
    Saba/GL/Model/MMD/GLMMDModelDrawContext.cpp
    Saba/GL/Model/MMD/GLMMDModelDrawer.cpp
)
set (
    GL_MODEL_MMD_HEADER
    Saba/GL/Model/MMD/GLMMDModel.h
    Saba/GL/Model/MMD/GLMMDModelDrawContext.h
    Saba/GL/Model/MMD/GLMMDModelDrawer.h
)

set (
    VIEWER_SOURCE
    Saba/Viewer/Camera.cpp
    Saba/Viewer/Grid.cpp
    Saba/Viewer/ModelDrawer.cpp
    Saba/Viewer/Viewer.cpp
    Saba/Viewer/ViewerCommand.cpp
    Saba/Viewer/ViewerContext.cpp
)
set (
    VIEWER_HEADER
    Saba/Viewer/Camera.h
    Saba/Viewer/Grid.h
    Saba/Viewer/ModelDrawer.h
    Saba/Viewer/Viewer.h
    Saba/Viewer/ViewerCommand.h
    Saba/Viewer/ViewerContext.h
)

set (GL3W_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/gl3w.c)

set (
    IMGUI_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_impl_glfw_gl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/ImGuizmo.cpp
)

SOURCE_GROUP(GL FILES ${GL_SOURCE} ${GL_HEADER})
SOURCE_GROUP(GL\\Model\\OBJ FILES ${GL_MODEL_OBJ_SOURCE} ${GL_MODEL_OBJ_HEADER})
SOURCE_GROUP(GL\\Model\\MMD FILES ${GL_MODEL_MMD_SOURCE} ${GL_MODEL_MMD_HEADER})
SOURCE_GROUP(Viewer FILES ${VIEWER_SOURCE} ${VIEWER_HEADER})
SOURCE_GROUP(GL3W FILES ${GL3W_SOURCE})
SOURCE_GROUP(IMGUI FILES ${IMGUI_SOURCE})

# SabaViewer

add_library(SabaViewer
    ${GL_SOURCE}
    ${GL_HEADER}
    ${GL_MODEL_OBJ_SOURCE}
    ${GL_MODEL_OBJ_HEADER}
    ${GL_MODEL_MMD_SOURCE}
    ${GL_MODEL_MMD_HEADER}
    ${VIEWER_SOURCE}
    ${VIEWER_HEADER}
    ${GL3W_SOURCE}
    ${IMGUI_SOURCE}
    ${IMGUIZMO_SOURCE}
    )
target_include_directories(SabaViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src
)
target_link_libraries(SabaViewer
    PUBLIC Saba
    PUBLIC glfw
    PRIVATE glslang
    PRIVATE OSDependent
    PRIVATE SPIRV
    PRIVATE OGLCompiler
    PRIVATE HLSL
    ${OPENGL_LIBRARY}
    )
