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
    net-tools \
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
RUN mkdir -p /opt \
    && git clone https://github.com/SESA/khpy.git /opt/khpy \
    && cd /opt/khpy \
    && git reset --hard 4a2dcc2 \
    && cp /opt/khpy/initd /etc/init.d/khpy \
    && chmod +x /etc/init.d/khpy

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

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    xz-utils

ENV DOCKER_BUCKET test.docker.com
ENV DOCKER_VERSION 1.12.0-rc2
ENV DOCKER_SHA256 6df54c3360f713370aa59b758c45185b9a62889899f1f6185a08497ffd57a39b

RUN set -x \
	&& curl -fSL "https://${DOCKER_BUCKET}/builds/Linux/x86_64/docker-$DOCKER_VERSION.tgz" -o docker.tgz \
	&& echo "${DOCKER_SHA256} *docker.tgz" | sha256sum -c - \
	&& tar -xzvf docker.tgz \
	&& mv docker/* /usr/local/bin/ \
	&& rmdir docker \
	&& rm docker.tgz \
	&& docker -v

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
WORKDIR /buildbotslavedata
CMD ["/usr/bin/supervisord"]
