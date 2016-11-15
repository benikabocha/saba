
file (GLOB SOURCE *.cpp)
file (GLOB HEADER *.h)

add_library(base ${SOURCE} ${HEADER})
