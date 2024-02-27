find_package(aws-lambda-runtime)
find_package(AWSSDK COMPONENTS s3)

add_executable(pathfind-request-processor "main.cpp")

target_link_libraries(pathfind-request-processor PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(pathfind-request-processor PRIVATE pathfinder)

target_compile_options(pathfind-request-processor PRIVATE
        "-Wall"
        "-Wextra"
        "-Wconversion"
        "-Wshadow"
        "-Wno-sign-conversion")

target_compile_features(pathfind-request-processor PRIVATE "cxx_std_11")

aws_lambda_package_target(pathfind-request-processor)