

add_library(pathfinder src/CollisionState.cpp src/Pathfinder.cpp src/PathfindRequest.cpp src/PathfindResult.cpp)
target_include_directories(pathfinder PUBLIC "${PROJECT_SOURCE_DIR}/pathfinder/include")

find_package(Boost 1.45.0 REQUIRED COMPONENTS system iostreams)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

target_include_directories(pathfinder PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(pathfinder ${Boost_LIBRARIES})