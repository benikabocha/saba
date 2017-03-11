if (SABA_ENABLE_TEST)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/include)

    file (GLOB SOURCE *.cpp)
    file (GLOB HEADER *.h)

    if (SABA_ENABLE_GL_TEST)
        file (GLOB GL_TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/GL.test/*.cpp)
        file (GLOB GL_TEST_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/GL.test/*.h)
    endif()

    add_executable(gtests
        ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/src/gtest-all.cc
        ${SOURCE}
        ${HEADER}
        ${GL_TEST_SOURCE}
        ${GL_TEST_HEADER}
    )

    set (gtests_LIBRARIES Saba SabaViewer)
    
    if (UNIX)
        find_package(Threads REQUIRED)
        list (APPEND gtests_LIBRARIES ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    endif ()

    target_include_directories(gtests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest
    )
    target_link_libraries(gtests ${gtests_LIBRARIES})
    if (SABA_ENABLE_GL_TEST)
        target_compile_definitions (gtests PRIVATE ENABLE_GLTEST=1)
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
