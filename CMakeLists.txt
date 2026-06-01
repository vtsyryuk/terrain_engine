
cmake_minimum_required(VERSION 3.16)

project(TerrainGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SRC_FILES ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(terrain_core ${SRC_FILES})
target_include_directories(terrain_core PUBLIC include)

if(WIN32)
    target_compile_definitions(terrain_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(MSVC)
    target_compile_options(terrain_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(terrain_core PRIVATE -Wall -Wextra)
endif()

add_executable(terrain_app src/main.cpp)
target_link_libraries(terrain_app terrain_core)

if(MSVC)
    target_compile_options(terrain_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(terrain_app PRIVATE -Wall -Wextra)
endif()
