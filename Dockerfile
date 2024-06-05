FROM gcc:14.1.0

ENV CMAKE_VERSION=3.29.4
ENV VCPKG_ROOT=/usr/src/vcpkg
ENV PATH=$PATH:$VCPKG_ROOT

COPY . /usr/src/mrm
WORKDIR /usr/src/mrm

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    autoconf-archive clang clang-tidy cpplint entr gdb ninja-build zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    ./cmake-${CMAKE_VERSION}-linux-x86_64.sh --skip-license --prefix=/usr/local && \
    rm cmake-${CMAKE_VERSION}-linux-x86_64.sh

RUN git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT && \
    cd $VCPKG_ROOT && \
    ./bootstrap-vcpkg.sh

RUN make clean build lint test
