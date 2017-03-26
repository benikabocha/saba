# glfw
if (SABA_FORCE_GLFW_BUILD)
  if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw)
    set (GLFW_BUILD_EXAMPLES off CACHE BOOL "" FORCE)
    set (GLFW_BUILD_TESTS off CACHE BOOL "" FORCE)
    set (GLFW_BUILD_DOCS off CACHE BOOL "" FORCE)
    set (GLFW_INSTALL off CACHE BOOL "" FORCE)
    add_subdirectory (glfw)
  else ()
    message (STATUS "glfw was not found.")
  endif ()
endif ()

# glslang
if (SABA_USE_GLSLANG)
  if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
    add_subdirectory (glslang)
  else ()
    message (STATUS "glslang was not found.")
  endif ()
endif ()

# fcpp
add_subdirectory (fcpp)

# lua
add_subdirectory (lua)
