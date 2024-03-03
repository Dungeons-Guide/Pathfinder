FROM alpine:3.19.1 AS build
LABEL authors="syeyoung"

RUN apk update && \
    apk add --no-cache \
        build-base=0.5-r3 \
        cmake=3.27.8-r0 \
        boost-dev=1.82.0-r3 \
        libcurl=8.5.0-r0 \
        openssl=3.1.4-r5 \
        git=2.43.0-r0

WORKDIR /
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
RUN apk add libcrypto3 crypto++ openssl-dev curl-dev zlib-dev

RUN cd aws-sdk-cpp  && mkdir build && cd /aws-sdk-cpp/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
              -DCMAKE_INSTALL_PREFIX=/out -DCMAKE_CXX_COMPILER=g++ \
              -DBUILD_ONLY="s3" && \
    make && make install

COPY . /build
COPY CMakeLists2.txt /build/CMakeLists.txt
RUN cd /build && ls && mkdir build && cd build && \
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_PREFIX_PATH=/out -DCMAKE_CXX_COMPILER=g++  && \
        make && make pathfind-request-processor-container

FROM alpine:3.19.1

RUN ls

RUN apk update && \
    apk add --no-cache \
    libstdc++ \
    boost1.82-iostreams \
    libcurl \
    zlib

COPY --from=build /build/build/container/pathfind-request-processor-container /app/

ENTRYPOINT ["/app/pathfind-request-processor-container"]