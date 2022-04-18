file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ReactorKun_SOURCE_DIR}/include/ReactorKun/*.hpp")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")

add_library(reactor_kun_core ${SOURCE_LIST} ${HEADER_LIST} ${UTF8_STRING_SOURCE})

target_include_directories(reactor_kun_core PUBLIC ../include/ReactorKun)

add_dependencies(reactor_kun_core reactor_parser)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
if(UNIX)
    find_package(Boost REQUIRED COMPONENTS thread stacktrace_addr2line)
endif(UNIX)
if(MSVC)
    find_package(Boost REQUIRED COMPONENTS thread stacktrace_windbg)
endif(MSVC)
target_link_libraries(reactor_kun_core PRIVATE ${CMAKE_THREAD_LIBS_INIT} TgBot dl
    ${Boost_LIBRARIES} SQLite::SQLite3 ${REACTOR_PARSER_LIB})

target_compile_features(reactor_kun_core PUBLIC cxx_std_20)
#target_compile_options(reactor_kun_core PUBLIC
#          $<$<$<CXX_COMPILER_ID:Clang,GNU>:$<CONFIG:Debug>>:-fsanitize=thread -fsanitize=undefined>
#          $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -pedantic -Werror>
#          $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#      )

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST})

