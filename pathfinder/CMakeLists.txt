option(PATHFINDER_CUDA "pathfinder: use CUDA optimizations" OFF)


if (PATHFINDER_CUDA)
    cmake_minimum_required(VERSION 3.18)  # for CMAKE_CUDA_ARCHITECTURES

    find_package(CUDAToolkit)
    if (CUDAToolkit_FOUND)
        message(STATUS "CUDA found")

        if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
            # 52 == lowest CUDA 12 standard
            # 60 == f16 CUDA intrinsics
            # 61 == integer CUDA intrinsics
            # 70 == compute capability at which unrolling a loop in mul_mat_q kernels is faster
            if (LLAMA_CUDA_F16 OR LLAMA_CUDA_DMMV_F16)
                set(CMAKE_CUDA_ARCHITECTURES "60;61;70") # needed for f16 CUDA intrinsics
            else()
                set(CMAKE_CUDA_ARCHITECTURES "52;61;70") # lowest CUDA 12 standard + lowest for integer intrinsics
                #set(CMAKE_CUDA_ARCHITECTURES "OFF") # use this to compile much faster, but only F16 models work
            endif()
        endif()
        message(STATUS "Using CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")

        enable_language(CUDA)
        set(PATHFINDER_EXTRA_LIBS CUDA::cudart)

        file(GLOB SOURCES_CUDA "src/cuda/*.cu")
        file(GLOB HEADERS_CUDA "src/cuda/*.h")

        add_compile_definitions(PATHFINDER_USE_CUDA)
    else()
        message(WARNING "CUDA not found")
    endif()
endif()

file(GLOB SOURCES "src/*.cpp")
#file(GLOB SOURCES "src/*.h")
file(GLOB HEADERS "include/*.h")


add_library(pathfinder
        ${SOURCES}
        ${HEADERS}
        ${HEADERS_CUDA}
        ${SOURCES_CUDA})
target_include_directories(pathfinder PUBLIC
        "${PROJECT_SOURCE_DIR}/pathfinder/include")

find_package(Boost 1.45.0 REQUIRED COMPONENTS system iostreams)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

target_include_directories(pathfinder PRIVATE ${Boost_INCLUDE_DIRS} ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
target_link_libraries(pathfinder ${Boost_LIBRARIES} ${PATHFINDER_EXTRA_LIBS})