cmake_minimum_required(VERSION 3.15)

project(LightFrame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LightFrame)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/runtime)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor)