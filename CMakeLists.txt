cmake_minimum_required(VERSION 3.27)
project(pathfinder2)

set(CMAKE_CXX_STANDARD 17)

add_executable(pathfinder2 main.cpp
            PathfindRequest.cpp
            PathfindRequest.h
            Pathfinder.cpp
            Pathfinder.h
            CollisionState.h
            CollisionState.cpp
            PathfindResult.h
            PathfindResult.cpp)


find_package(Boost 1.45.0 REQUIRED COMPONENTS system iostreams)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(pathfinder2 ${Boost_LIBRARIES})
