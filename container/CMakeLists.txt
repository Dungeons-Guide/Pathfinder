find_package(ZLIB REQUIRED)
find_package(AWSSDK COMPONENTS s3)

add_executable(pathfind-request-processor-container "main.cpp")

target_link_libraries(pathfind-request-processor-container PRIVATE ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(pathfind-request-processor-container PRIVATE pathfinder)

target_compile_options(pathfind-request-processor-container PRIVATE
        "-Wall"
        "-Wextra"
        "-Wconversion"
        "-Wshadow"
        "-Wno-sign-conversion")

target_compile_features(pathfind-request-processor-container PRIVATE "cxx_std_11")