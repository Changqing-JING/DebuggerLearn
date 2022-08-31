cmake_minimum_required(VERSION 3.5)
project(DebuggerLearn)

if(MSVC)
    set(CMAKE_SUPPRESS_REGENERATION ON)
    add_subdirectory(WindowsDebugger)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "QCC")
    add_definitions(-D_QNX_SOURCE)
    add_subdirectory(QNXDebugger)
elseif((UNIX AND(NOT APPLE)))
    add_subdirectory(LinuxDebugger)
endif()

aux_source_directory(src sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})