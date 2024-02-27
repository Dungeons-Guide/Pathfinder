FROM --platform=linux/x86_64 public.ecr.aws/lambda/provided:al2023.2024.02.07.17
LABEL authors="syeyoung"

RUN dnf install -y gcc-c++ libcurl-devel cmake3 make;
RUN dnf install -y boost-devel zlib-devel openssl-devel;
RUN dnf install -y git;
WORKDIR /
RUN git clone https://github.com/awslabs/aws-lambda-cpp.git && \
    cd aws-lambda-cpp && mkdir build && cd /aws-lambda-cpp/build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++ && \
    make && make install
WORKDIR /
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
    cd aws-sdk-cpp  && mkdir build && cd /aws-sdk-cpp/build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
              -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++ \
              -DBUILD_ONLY="s3" && \
    make && make install



COPY . /build
RUN cd /build && mkdir build && cd build && \
        cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_PREFIX_PATH=/out -DCMAKE_CXX_COMPILER=g++  && \
        make && make aws-lambda-package-pathfind-request-processor


ENTRYPOINT ["/build/cmake-build-debug/lambda/pathfind-request-processor"]