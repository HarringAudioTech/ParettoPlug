cmake_minimum_required(VERSION 3.22)

project(ParettoBench)

set(SOURCE_FILES
    source/ParettoBench.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE ParettoCore)

set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")
