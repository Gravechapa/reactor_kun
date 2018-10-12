cmake_minimum_required(VERSION 3.11.4)

include(ExternalProject)
ExternalProject_Add(
    reactor_parser
    GIT_REPOSITORY     https://github.com/Gravechapa/reactor_parser.git
    GIT_TAG            master
    GIT_SHALLOW        1
    BUILD_COMMAND      cargo build --release
    SOURCE_DIR         "${CMAKE_BINARY_DIR}/reactor_parser"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/reactor_parser"
    INSTALL_COMMAND    ""
    CONFIGURE_COMMAND  ""
)


include_directories("${CMAKE_BINARY_DIR}/reactor_parser/headers")
set(REACTOR_PARSER_LIB "${CMAKE_BINARY_DIR}/reactor_parser/target/release/libreactor_parser.so")
