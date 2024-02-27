FROM --platform=linux/x86_64 gcc:latest
LABEL authors="syeyoung"

RUN apt-get update; \
    apt-get install -y cmake; \
    mkdir -p /usr/src;


COPY . /usr/src

RUN set -ex;              \
    cd /usr/src;  \
    cmake .; make; make install \

ENTRYPOINT ["top", "-b"]