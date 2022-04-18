FetchContent_Declare(
    reactor_parser
    GIT_REPOSITORY     https://github.com/Gravechapa/reactor_parser.git
    GIT_TAG            master
    GIT_SHALLOW        1
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/reactor_parser"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/reactor_parser"
    INSTALL_COMMAND    ""
    CONFIGURE_COMMAND  ""
)

FetchContent_GetProperties(reactor_parser)
if(NOT reactor_parser_POPULATED)
    FetchContent_Populate(reactor_parser)

    include_directories("${reactor_parser_SOURCE_DIR}/headers")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_target(reactor_parser
                    COMMAND cargo build --target-dir="\"${CMAKE_BINARY_DIR}/reactor_parser\""
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/thirdparty/reactor_parser")
        set(REACTOR_PARSER_LIB "${CMAKE_BINARY_DIR}/reactor_parser/debug/libreactor_parser.so")
    else()
        add_custom_target(reactor_parser
                    COMMAND cargo build --release --target-dir="\"${CMAKE_BINARY_DIR}/reactor_parser\""
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/thirdparty/reactor_parser")
        set(REACTOR_PARSER_LIB "${CMAKE_BINARY_DIR}/reactor_parser/release/libreactor_parser.so")
    endif()
endif()
