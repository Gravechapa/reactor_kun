set(CPR_USE_SYSTEM_CURL ON CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_Declare(
    cpr
    GIT_REPOSITORY     https://github.com/libcpr/cpr.git
    GIT_TAG            1.14.2
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/cpr"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/cpr-bin"
)
FetchContent_MakeAvailable(cpr)
include_directories("${cpr_SOURCE_DIR}/include" "${cpr_BINARY_DIR}/cpr_generated_includes")
