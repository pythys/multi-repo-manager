FROM buildpack-deps:bullseye-scm

ENV CMAKE_VERSION=3.29.5
ENV VCPKG_ROOT=/usr/src/vcpkg
ENV PATH=$PATH:$VCPKG_ROOT

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    clang \
    make \
    python3 \
    python3-pip \
    zip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN pip3 install cpplint

RUN CMAKE_BASE=https://github.com/Kitware/CMake/releases/download/ && \
    CMAKE_FILE=cmake-${CMAKE_VERSION}-linux-x86_64.sh && \
    CMAKE_URL=${CMAKE_BASE}/v${CMAKE_VERSION}/${CMAKE_FILE} && \
    wget $CMAKE_URL && \
    chmod +x ${CMAKE_FILE} && \
    ./${CMAKE_FILE} --skip-license --prefix=/usr/local && \
    rm ${CMAKE_FILE}

RUN git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT && \
    cd $VCPKG_ROOT && \
    ./bootstrap-vcpkg.sh -disableMetrics

COPY . /usr/src/mrm

RUN cd /usr/src/mrm && \
    make clean build lint test && \
    cp ./build/mrm/mrm /usr/local/bin && \
    rm -rf /usr/src/mrm
