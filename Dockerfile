# https://github.com/Kuyoh/docker-vcpkg
FROM kuyoh/vcpkg

COPY . /usr/src/mrm
WORKDIR /usr/src/mrm

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    clang cmake cpplint entr gdb ninja-build && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN make clean build lint test

