if (TARGET gmock)
    message(STATUS "Google Mock found - building tests")

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../external/googletest/googletest/include)

    file (GLOB SOURCE *.cpp)
    file (GLOB HEADER *.h)

    if (SABA_ENABLE_GL_TEST)
        file (GLOB GL_TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/GL.test/*.cpp)
        file (GLOB GL_TEST_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/GL.test/*.h)
    endif()

    add_executable(gtests ${SOURCE} ${HEADER} ${GL_TEST_SOURCE} ${GL_TEST_HEADER})
    target_link_libraries(gtests
        Saba
        gtest
        glfw
        ${OPENGL_LIBRARY}
    )
    if (SABA_ENABLE_GL_TEST)
        target_link_libraries(gtests
            glfw
            ${OPENGL_LIBRARY}
        )
        target_compile_definitions(gtests
            PRIVATE ENABLE_GLTEST=1
        )
    endif()

    set(GLSLANG_TEST_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../gtests/Data")
    target_compile_definitions(
        gtests
        PRIVATE TEST_DATA_PATH="${GLSLANG_TEST_DIRECTORY}"
    )
    
    add_test(
        NAME all_test
        COMMAND $<TARGET_FILE:gtests>
    )
endif()



