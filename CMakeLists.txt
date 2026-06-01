
cmake_minimum_required(VERSION 3.16)

project(TerrainGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(terrain_core ${SRC_FILES})

add_executable(terrain_app src/main.cpp)
target_link_libraries(terrain_app terrain_core)
