# GL
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

# OBJ
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

# XFile
set (
    GL_MODEL_XFILE_SOURCE
    Saba/GL/Model/XFile/GLXFileModel.cpp
    Saba/GL/Model/XFile/GLXFileModelDrawContext.cpp
    Saba/GL/Model/XFile/GLXFileModelDrawer.cpp
)
set (
    GL_MODEL_XFILE_HEADER
    Saba/GL/Model/XFile/GLXFileModel.h
    Saba/GL/Model/XFile/GLXFileModelDrawContext.h
    Saba/GL/Model/XFile/GLXFileModelDrawer.h
)

# MMD
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

# Viewer
set (
    VIEWER_SOURCE
    Saba/Viewer/Camera.cpp
    Saba/Viewer/Light.cpp
    Saba/Viewer/Grid.cpp
    Saba/Viewer/ModelDrawer.cpp
    Saba/Viewer/Viewer.cpp
    Saba/Viewer/ViewerCommand.cpp
    Saba/Viewer/ViewerContext.cpp
    Saba/Viewer/CameraOverrider.cpp
    Saba/Viewer/VMDCameraOverrider.cpp
    Saba/Viewer/ShadowMap.cpp
)
set (
    VIEWER_HEADER
    Saba/Viewer/Camera.h
    Saba/Viewer/Light.h
    Saba/Viewer/Grid.h
    Saba/Viewer/ModelDrawer.h
    Saba/Viewer/Viewer.h
    Saba/Viewer/ViewerCommand.h
    Saba/Viewer/ViewerContext.h
    Saba/Viewer/CameraOverrider.h
    Saba/Viewer/VMDCameraOverrider.h
    Saba/Viewer/ShadowMap.h
)

# gl3w
set (GL3W_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/gl3w.c)

# imgui
set (
    IMGUI_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src/ImGuizmo.cpp
)

# Group
SOURCE_GROUP(GL FILES ${GL_SOURCE} ${GL_HEADER})
SOURCE_GROUP(GL\\Model\\OBJ FILES ${GL_MODEL_OBJ_SOURCE} ${GL_MODEL_OBJ_HEADER})
SOURCE_GROUP(GL\\Model\\XFile FILES ${GL_MODEL_XFILE_SOURCE} ${GL_MODEL_XFILE_HEADER})
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
    ${GL_MODEL_XFILE_SOURCE}
    ${GL_MODEL_XFILE_HEADER}
    ${GL_MODEL_MMD_SOURCE}
    ${GL_MODEL_MMD_HEADER}
    ${VIEWER_SOURCE}
    ${VIEWER_HEADER}
    ${GL3W_SOURCE}
    ${IMGUI_SOURCE}
    ${IMGUIZMO_SOURCE}
)
target_include_directories(SabaViewer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/fcpp
)

set (SabaViewer_LIBRARIES Saba fcpp ${OPENGL_LIBRARY} lua)

# Link GLFW library
if (SABA_FORCE_GLFW_BUILD)
    list (APPEND SabaViewer_LIBRARIES glfw)
else ()
    list (APPEND SabaViewer_LIBRARIES ${GLFW_LIBRARIES})
endif ()

# Use glslang library
if (SABA_USE_GLSLANG)
    include_directories(${PROJECT_SOURCE_DIR}/external/glslang)
    target_compile_definitions(SabaViewer
        PRIVATE USE_GLSLANG=1
    )
    list (APPEND SabaViewer_LIBRARIES
        PRIVATE glslang
        PRIVATE OSDependent
        PRIVATE SPIRV
        PRIVATE OGLCompiler
        PRIVATE HLSL
    )
endif()

target_link_libraries(SabaViewer ${SabaViewer_LIBRARIES})
