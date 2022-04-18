FetchContent_Declare(
    utf8_string
    GIT_REPOSITORY     https://github.com/Gumichan01/utf8_string.git
    GIT_TAG            cbbe28c4adfcc293f6784cbd8c209c4c86484864
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/utf8_string"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/utf8_string-bin"
    INSTALL_COMMAND    ""
    CONFIGURE_COMMAND  ""
)

FetchContent_GetProperties(utf8_string)
if(NOT utf8_string_POPULATED)
    FetchContent_Populate(utf8_string)

    include_directories("${utf8_string_SOURCE_DIR}/src")
    set(UTF8_STRING_SOURCE "${utf8_string_SOURCE_DIR}/src/utf8_iterator.cpp" "${utf8_string_SOURCE_DIR}/src/utf8_string.cpp")
endif()
