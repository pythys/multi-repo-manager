FROM debian:bookworm AS build

ENV CONAN_VERSION=2.26.2
ENV MAKEFLAGS="-j$(nproc)"

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    clang-16 \
    clang-format-16 \
    clang-tidy-16 \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    unzip \
    zip && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-16 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-16 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-16 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-16 100 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN python3 -m pip install --no-cache-dir --break-system-packages \
    cmakelang \
    conan==${CONAN_VERSION}

COPY . /usr/src/mrm

RUN cd /usr/src/mrm && \
    make clean && \
    make test lint && \
    make install

RUN strip /usr/local/bin/mrm

FROM gcr.io/distroless/cc-debian12

COPY --from=build /usr/local/bin/mrm /usr/local/bin/mrm

WORKDIR /repos

ENTRYPOINT ["/usr/local/bin/mrm"]
