FROM debian:bookworm AS build

ENV CONAN_VERSION=2.26.2

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    clang-format \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    unzip \
    zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN python3 -m pip install --no-cache-dir --break-system-packages \
    cmakelang \
    conan==${CONAN_VERSION}

COPY . /usr/src/mrm

RUN cd /usr/src/mrm && \
    make clean test lint && \
    make install

RUN strip /usr/local/bin/mrm

FROM gcr.io/distroless/cc-debian12

COPY --from=build /usr/local/bin/mrm /usr/local/bin/mrm

ENTRYPOINT ["/usr/local/bin/mrm"]
