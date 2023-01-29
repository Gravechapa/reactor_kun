add_executable(reactor_kun main.cpp)
if(MSVC)
    set_target_properties(reactor_kun PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

target_link_libraries(reactor_kun PRIVATE reactor_kun_core)
