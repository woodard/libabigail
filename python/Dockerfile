FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential \
    libelf-dev \
    libzip-dev \
    autoconf \
    libtool \
    pkg-config \
    libxml2 \
    libxml2-dev \
    elfutils \
    doxygen \
    wget \
    git \
    libdw-dev \
    elfutils \
    python3-lxml \
    python3 \
    python3-dev \
    python3-sphinx \
    python3-pip

# Yes we are cheating with xmltodict :)
RUN pip3 install ipython numpy && \
    git clone https://github.com/llnl/shroud && \
    cd shroud && \
    pip3 install .
WORKDIR /code
COPY . /code
RUN ldconfig && \
    autoreconf -i && \
    mkdir build && \
    cd build && \
    ../configure --prefix=/usr/local  && \
    make all install && \
    ldconfig
