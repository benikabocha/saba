
file (GLOB BASE_SOURCE Saba/Base/*.cpp)
file (GLOB BASE_HEADER Saba/Base/*.h)

file (GLOB GL_SOURCE Saba/GL/*.cpp)
file (GLOB GL_HEADER Saba/GL/*.h)

file (GLOB GL3W_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../external/gl3w/src/*.c)

add_library(
    Saba
    ${BASE_SOURCE}
    ${BASE_HEADER}
    ${GL_SOURCE}
    ${GL_HEADER}
    ${GL3W_SOURCE}
)
target_link_libraries(Saba glslang OSDependent SPIRV OGLCompiler HLSL)

SOURCE_GROUP(Base FILES ${BASE_SOURCE} ${BASE_HEADER})
SOURCE_GROUP(GL FILES ${GL_SOURCE} ${GL_HEADER})
SOURCE_GROUP(GL3W FILES ${GL3W_SOURCE})

