set(HTML_BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_Declare(
    htmlparser
    GIT_REPOSITORY     https://github.com/mylogin/htmlparser.git
    GIT_TAG            f468bb3eb6ed2ac38b7c5cb8e6789346f888c69a
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/htmlparser"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/htmlparser-bin"
)
FetchContent_MakeAvailable(htmlparser)
include_directories("${htmlparser_SOURCE_DIR}")
