FetchContent_Declare(
    plog
    GIT_REPOSITORY     https://github.com/SergiusTheBest/plog.git
    GIT_TAG            1.1.6
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/plog"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/plog-bin"
)
FetchContent_MakeAvailable(plog)
include_directories("${plog_SOURCE_DIR}/include")
