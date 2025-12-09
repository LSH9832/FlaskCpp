cmake_minimum_required(VERSION 3.10)

project(flaskcpp_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -fPIC -pthread -w ${CMAKE_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE Release) # Debug

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/install/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/install/lib/")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

link_directories(${CMAKE_SOURCE_DIR}/install/lib)

# need yaml-cpp
add_executable(flaskcpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flaskcpp.cpp
)

target_link_libraries(flaskcpp
    FlaskCpp
    yaml-cpp
)