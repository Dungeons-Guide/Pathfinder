FROM alpine:3.17.0 AS build
LABEL authors="syeyoung"

RUN apk update && \
    apk add --no-cache \
        build-base=0.5-r3 \
        cmake=3.28.3-r0 \
        boost-dev=1.82.0-r4 \
        libcurl=8.6.0-r0 \
        openssl=3.1.5-r5 \
        git=2.44.0-r0

WORKDIR /
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp \

RUN cd aws-sdk-cpp  && mkdir build && cd /aws-sdk-cpp/build && \
    cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
              -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++ \
              -DBUILD_ONLY="s3" && \
    make && make install

COPY . /build
RUN cd /build && mkdir build && cd build && \
        cmake3 .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_PREFIX_PATH=/out -DCMAKE_CXX_COMPILER=g++  && \
        make && make pathfind-request-processor-container


ENTRYPOINT ["/build/cmake-build-debug/lambda/pathfind-request-processor"]