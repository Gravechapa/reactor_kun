FetchContent_Declare(
    utf8_string
    GIT_REPOSITORY     https://github.com/Gumichan01/utf8_string.git
    GIT_TAG            4e677cd3d7986dc1406f3b50e64ecaec68dd6b88
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/utf8_string"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/utf8_string-bin"
)

FetchContent_MakeAvailable(utf8_string)
include_directories("${utf8_string_SOURCE_DIR}/src")
add_library(utf8_string "${utf8_string_SOURCE_DIR}/src/utf8_iterator.cpp" "${utf8_string_SOURCE_DIR}/src/utf8_string.cpp"
                        "${utf8_string_SOURCE_DIR}/src/utf8_iterator.hpp" "${utf8_string_SOURCE_DIR}/src/utf8_string.hpp")
set_target_properties(utf8_string PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY "${utf8_string_BINARY_DIR}"
                      LIBRARY_OUTPUT_DIRECTORY "${utf8_string_BINARY_DIR}")
