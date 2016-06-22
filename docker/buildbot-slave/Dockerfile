FROM ubuntu:16.04

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    autoconf \
    automake \
    bridge-utils \
    build-essential \
    clang-format-3.8 \
    cmake  \
    curl \
    dnsmasq \
    git \
    libboost-coroutine-dev  \
    libboost-dev  \
    libboost-filesystem-dev  \
    libfdt-dev \
    libglib2.0-dev \
    libtbb-dev \
    libtool  \
    pkg-config \
    python-dev \
    python-pexpect \
    python-pip \
    software-properties-common \
    supervisor \
    wget \
    zlib1g-dev

#
# QEMU
#
RUN git clone -b pin-threads https://github.com/SESA/qemu.git /tmp/qemu
WORKDIR /tmp/qemu
RUN git submodule update --init pixman
RUN ./configure --target-list=x86_64-softmmu --enable-vhost-net --enable-kvm
RUN make -j
RUN make install

#
# Khpy
#
RUN mkdir -p /opt
RUN git clone https://github.com/SESA/khpy.git /opt/khpy
RUN cp /opt/khpy/initd /etc/init.d/khpy
RUN chmod +x /etc/init.d/khpy

#
# Build and install capnproto
#
RUN wget -O /tmp/capnproto.tar.gz https://github.com/sandstorm-io/capnproto/archive/v0.4.0.tar.gz
WORKDIR /tmp
RUN tar -xf /tmp/capnproto.tar.gz
WORKDIR /tmp/capnproto-0.4.0/c++
RUN autoreconf -i
RUN CXXFLAGS=-fpermissive ./configure
RUN make -j
RUN make install

# BuildBot
RUN pip install buildbot_slave
RUN groupadd -g 5001 buildbot
RUN useradd -u 5001 -g buildbot buildbot

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
WORKDIR /buildbotslavedata
CMD ["/usr/bin/supervisord"]
