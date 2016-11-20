if (TARGET gmock)
    message(STATUS "Google Mock found - building tests")

    file (GLOB SOURCE *.cpp)

    add_executable(gtests ${SOURCE})
    target_link_libraries(
        gtests
        Saba
        gtest_main
    )

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



