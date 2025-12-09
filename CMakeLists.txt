cmake_minimum_required(VERSION 3.10)

project(FlaskCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -fPIC -pthread -w ${CMAKE_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE Release) # Debug

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/install/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/install/lib/")


include_directories(
    ./include
)


add_library(FlaskCpp SHARED
    src/FlaskCpp.cpp
    src/TemplateEngine.cpp
    src/ThreadPool.cpp
    src/FlaskTypes.cpp
    src/utils/file.cpp
    src/utils/response.cpp
)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/demo)

