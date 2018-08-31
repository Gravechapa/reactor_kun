cmake_minimum_required(VERSION 3.11.4)
set(PROJECT Reactor_kun)
project(${PROJECT})
message(STATUS "Start configuring project ${Reactor_kun}")

set(EXECUTABLE ${PROJECT})
set(SOURCE_ROOT src)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXTERNAL_PROJECTS_DIR "cmake_externalprojects")

############################TGbot#####################################
message("Configure TGbot")
include("${EXTERNAL_PROJECTS_DIR}/TGBot.cmake")
#####################################################################

message(STATUS "Starting collecting files")
aux_source_directory(${SOURCE_ROOT} SOURCES)
file (GLOB_RECURSE SUB_SOURCES "${SOURCE_ROOT}/classes/*.c*")
set(SOURCES ${SOURCES} ${SUB_SOURCES})

file (GLOB_RECURSE HEADERS "${SOURCE_ROOT}/headers/*.h*")

source_group(Classes FILES ${SUB_SOURCES})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
add_definitions(-DHAVE_CURL)
include_directories(${SOURCE_ROOT}/headers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${EXECUTABLE} ${SOURCES} ${HEADERS})
if(MSVC)
    set_target_properties(${EXECUTABLE} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    
    target_compile_options(${EXECUTABLE} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX>)
    
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    target_compile_options(${EXECUTABLE} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic>)
endif()

target_link_libraries(${EXECUTABLE} ${CMAKE_THREAD_LIBS_INIT} TgBot)


#############Copies data files to a binary directory####################################
set (DATA_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/")
file (GLOB_RECURSE CONFIGS RELATIVE "${DATA_FILES_DIR}" "${DATA_FILES_DIR}/configs/*")

foreach(CONFIG ${CONFIGS})
    configure_file(${DATA_FILES_DIR}/${CONFIG}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG} COPYONLY)
endforeach(CONFIG)
#########################################################################################
