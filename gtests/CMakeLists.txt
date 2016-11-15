if (TARGET gmock)
    message(STATUS "Google Mock found - building tests")

    file (GLOB SOURCE *.cpp)

    add_executable(gtests ${SOURCE})
    target_link_libraries(
        gtests
        base
        gtest_main
    )

    add_test(
        NAME all_test
        COMMAND $<TARGET_FILE:gtests>
    )
endif()



