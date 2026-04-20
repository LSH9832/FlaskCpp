cmake_minimum_required(VERSION 3.10)

project(FlaskCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_BUILD_TYPE Release) # Debug
if(WIN32)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
set(CMAKE_CXX_FLAGS "-O3 -fPIC -pthread -w ${CMAKE_CXX_FLAGS}")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(FlaskCpp SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FlaskCpp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TemplateEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FlaskTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/response.cpp
)

if(WIN32)
target_link_libraries(FlaskCpp
    ws2_32
)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/src/demo)

