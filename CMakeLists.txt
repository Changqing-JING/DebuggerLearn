cmake_minimum_required(VERSION 3.5)
project(DebuggerLearn)

if(MSVC)
set(CMAKE_SUPPRESS_REGENERATION ON)
endif()

aux_source_directory(src sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})