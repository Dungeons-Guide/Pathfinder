FROM --platform=linux/x86_64 ubuntu
LABEL authors="syeyoung"

RUN apt-get update; \
    apt-get install -y cmake build-essential;
RUN apt-get install -y libboost-all-dev;
RUN mkdir -p /usr/src/src;


COPY pathfinder/src/* /usr/src/src
COPY pathfinder/CMakeLists.txt /usr/src

RUN set -ex;              \
    cd /usr/src;  \
    cmake .; make;

ENTRYPOINT ["top", "-b"]