add_executable(reactor_kun main.cpp)
target_compile_features(reactor_kun PRIVATE cxx_std_20)
if(MSVC)
    set_target_properties(reactor_kun PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

check_ipo_supported(RESULT result)
if(result)
  set_target_properties(reactor_kun PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(reactor_kun PRIVATE reactor_kun_core)
