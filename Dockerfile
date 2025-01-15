FROM debian:bookworm

ENV CMAKE_VERSION=3.31.3
ENV VCPKG_ROOT=/usr/src/vcpkg
ENV PATH=$PATH:$VCPKG_ROOT

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    clang \
    clang-tidy \
    cpplint \
    curl \
    git \
    libgit2-dev \
    ninja-build \
    pkg-config \
    unzip \
    zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN CMAKE_BASE=https://github.com/Kitware/CMake/releases/download/ && \
    CMAKE_FILE=cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    CMAKE_URL=${CMAKE_BASE}/v${CMAKE_VERSION}/${CMAKE_FILE} && \
    curl -LO $CMAKE_URL && \
    chmod +x ${CMAKE_FILE} && \
    ./${CMAKE_FILE} --skip-license --prefix=/usr/local && \
    rm ${CMAKE_FILE}

RUN git clone --depth 1 https://github.com/microsoft/vcpkg $VCPKG_ROOT && \
    cd $VCPKG_ROOT && \
    ./bootstrap-vcpkg.sh -disableMetrics

COPY . /usr/src/mrm

RUN cd /usr/src/mrm && \
    make clean test lint && \
    cp ./build/mrm/mrm /usr/local/bin
