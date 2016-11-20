
file (GLOB BASE_SOURCE Saba/Base/*.cpp)
file (GLOB BASE_HEADER Saba/Base/*.h)

add_library(
    Saba
    ${BASE_SOURCE}
    ${BASE_HEADER}
)

SOURCE_GROUP(Base FILES ${BASE_SOURCE} ${BASE_HEADER})
