cmake_minimum_required(VERSION 3.2)
project(pathfinder)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE  "-O3")

add_subdirectory("${PROJECT_SOURCE_DIR}/pathfinder")
add_subdirectory("${PROJECT_SOURCE_DIR}/lambda")
add_subdirectory("${PROJECT_SOURCE_DIR}/cli")
add_subdirectory("${PROJECT_SOURCE_DIR}/container")

#add_subdirectory(binary)
