FROM kernsuite/base:4

RUN export DEBIAN_FRONTEND=noninteractive && apt-get update && \
    apt-get install -y \
    cmake \
    build-essential \
    pkg-config \
    casacore-data casacore-dev \
    libblas-dev liblapack-dev \
    libpython-dev \
    libboost-date-time-dev libboost-signals-dev libboost-python-dev libboost-test-dev libboost-system-dev libboost-filesystem-dev \
    libgtkmm-3.0-dev \
    libcfitsio-dev \
    libfftw3-dev \
    libxml2-dev \
    libgsl-dev \
    libpng-dev

ADD . /src
WORKDIR /src

RUN mkdir /build && cd /build && cmake ../src

RUN cd /build && make -j2 && make install && make check -j2
