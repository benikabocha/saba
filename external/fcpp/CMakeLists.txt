file(GLOB FCPP_SOURCE *.c *.h)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(fcpp ${FCPP_SOURCE})
