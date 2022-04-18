add_executable(reactor_kun main.cpp)
target_compile_features(reactor_kun PRIVATE cxx_std_20)
target_link_libraries(reactor_kun PRIVATE reactor_kun_core)
if(MSVC)
    set_target_properties(reactor_kun PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
