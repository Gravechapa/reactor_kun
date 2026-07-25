add_executable(reactor_kun main.cpp)
if(MSVC)
    set_target_properties(reactor_kun PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

find_package(CURL REQUIRED)
if(UNIX)
    find_package(Boost REQUIRED COMPONENTS stacktrace_addr2line)
endif(UNIX)
if(MSVC)
    find_package(Boost REQUIRED COMPONENTS stacktrace_windbg)
endif(MSVC)
target_link_libraries(reactor_kun PRIVATE reactor_kun_core ${Boost_LIBRARIES} CURL::libcurl)
