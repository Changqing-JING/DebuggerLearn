project(DebugTarget)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})