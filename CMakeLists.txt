cmake_minimum_required(VERSION 3.5)
project(pathfinder)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("${PROJECT_SOURCE_DIR}/pathfinder")
add_subdirectory("${PROJECT_SOURCE_DIR}/lambda")
add_subdirectory("${PROJECT_SOURCE_DIR}/cli")
#add_subdirectory(binary)
