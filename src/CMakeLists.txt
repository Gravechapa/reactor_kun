file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ReactorKun_SOURCE_DIR}/include/ReactorKun/*.hpp")
file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE MODULE_LIST CONFIGURE_DEPENDS "*.cppm")

add_library(reactor_kun_core ${SOURCE_LIST} ${HEADER_LIST} ${UTF8_STRING_SOURCE})

target_sources(reactor_kun_core PRIVATE FILE_SET modules TYPE CXX_MODULES FILES "${MODULE_LIST}")

target_include_directories(reactor_kun_core PUBLIC ../include/ReactorKun)

target_precompile_headers(reactor_kun_core PUBLIC ${HEADER_LIST})

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

target_link_libraries(reactor_kun_core PRIVATE ${CMAKE_THREAD_LIBS_INIT} Td::TdStatic dl
                      SQLite3::SQLite3 cpr::cpr htmlparser utf8_string)

target_compile_options(reactor_kun_core PUBLIC
          #$<$<$<CXX_COMPILER_ID:Clang,GNU>:$<CONFIG:Debug>>:-fsanitize=thread -fsanitize=undefined>
          $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -pedantic>
          $<$<CXX_COMPILER_ID:MSVC>:/W4>
      )

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST})

