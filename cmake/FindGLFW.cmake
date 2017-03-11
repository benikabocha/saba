# GLFW_FOUND
# GLFW_INCLUDE_DIR
# GLFW_LIBRARIES
#

find_path (GLFW_INCLUDE_DIR
    NAMES
        GLFW/glfw3.h
    PATHS
        "${GLFW_ROOT}/include"
        /usr/X11R6/include
        /usr/include/X11
        /opt/graphics/OpenGL/include
        /opt/graphics/OpenGL/contrib/libglfw
        /usr/local/include
        /usr/include/GL
        /usr/include
    DOC
        "The directory where GL/glfw.h resides"
)

if (WIN32)
    if (MSVC11)
        find_library (GLFW_LIBRARIES
            NAMES
                glfw3
            PATHS
                "${GLFW_ROOT}/lib"
                "${GLFW_ROOT}/lib-vc2012"
            DOCS
                "The GLFW library"
        )
    elseif (MSVC12)
        find_library (GLFW_LIBRARIES
            NAMES
                glfw3
            PATHS
                "${GLFW_ROOT}/lib"
                "${GLFW_ROOT}/lib-vc2013"
            DOCS
                "The GLFW library"
        )
    elseif (MSVC14)
        find_library (GLFW_LIBRARIES
            NAMES
                glfw3
            PATHS
                "${GLFW_ROOT}/lib"
                "${GLFW_ROOT}/lib-vc2015"
            DOCS
                "The GLFW library"
        )
    elseif (MINGW)
        if (CMAKE_CL_64)
            find_library (GLFW_LIBRARIES
                NAMES
                    glfw3
                PATHS
                    "${GLFW_ROOT}/lib"
                    "${GLFW_ROOT}/lib-mingw-w64"
                DOCS
                    "The GLFW library"
            )
        else ()
            find_library (GLFW_LIBRARIES
                NAMES
                    glfw3
                PATHS
                    "${GLFW_ROOT}/lib"
                    "${GLFW_ROOT}/lib-mingw"
                DOCS
                    "The GLFW library"
            )
        endif ()
    else()
        find_library (GLFW_LIBRARIES
            NAMES
                glfw3
            PATHS
                "${GLFW_ROOT}/lib"
            DOCS
                "The GLFW library"
        )
    endif()
else ()
    find_library (GLFW_LIBRARIES
        NAMES
            glfw
            glfw3
        PATHS
            "${GLFW_ROOT}/lib"
            "${GLFW_ROOT}/lib/x11"
            /usr/lib64
            /usr/lib
            /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            /usr/local/lib64
            /usr/local/lib
            /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            /usr/X11R6/lib
        DOCS
            "The GLFW library"
    )
    if (APPLE)
        list (APPEND GLFW_LIBRARIES
            "-framework Cocoa"
            "-framework CoreVideo"
            "-framework IOKit"
        )
    endif ()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW DEFAULT_MSG GLFW_LIBRARIES GLFW_INCLUDE_DIR)

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_LIBRARIES)
