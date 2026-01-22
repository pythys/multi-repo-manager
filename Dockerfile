FROM debian:bookworm AS build

ENV CMAKE_VERSION=4.2.1
ENV VCPKG_ROOT=/usr/src/vcpkg
ENV PATH=$PATH:$VCPKG_ROOT

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    curl \
    git \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    unzip \
    zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    pip install --break-system-packages cpplint

RUN CMAKE_BASE=https://github.com/Kitware/CMake/releases/download/ && \
    CMAKE_FILE=cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    CMAKE_URL=${CMAKE_BASE}/v${CMAKE_VERSION}/${CMAKE_FILE} && \
    curl -LO $CMAKE_URL && \
    chmod +x ${CMAKE_FILE} && \
    ./${CMAKE_FILE} --skip-license --prefix=/usr/local && \
    rm ${CMAKE_FILE}

RUN git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT && \
    cd $VCPKG_ROOT && \
    ./bootstrap-vcpkg.sh -disableMetrics

COPY . /usr/src/mrm

RUN cd /usr/src/mrm && \
    make clean test lint && \
    make install

RUN strip /usr/local/bin/mrm

FROM gcr.io/distroless/cc-debian12

COPY --from=build /usr/local/bin/mrm /usr/local/bin/mrm

ENTRYPOINT ["/usr/local/bin/mrm"]
