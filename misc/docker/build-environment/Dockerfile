FROM ubuntu:xenial

RUN apt-get update -yqq
RUN apt-get install -yqq \
autoconf \
automake \
build-essential \
cmake  \
curl  \
git \
libboost-coroutine-dev  \
libboost-dev  \
libboost-filesystem-dev  \
libfdt-dev \
libglib2.0-dev \
libtbb-dev \
libtool  \
libtool \
pkg-config \
software-properties-common \
texinfo \
wget \
zlib1g-dev

RUN mkdir -p /tmp

# DOCKER
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

# QEMU
RUN git clone -b pin-threads https://github.com/SESA/qemu.git /tmp/qemu
WORKDIR /tmp/qemu
RUN git submodule update --init pixman
RUN ./configure --target-list=x86_64-softmmu --enable-vhost-net --enable-kvm
RUN make -j
RUN make install
RUN make clean

# CAPNPROTO
RUN wget -O /tmp/capnproto.tar.gz https://github.com/sandstorm-io/capnproto/archive/v0.4.0.tar.gz
WORKDIR /tmp
RUN tar -xf /tmp/capnproto.tar.gz 
WORKDIR /tmp/capnproto-0.4.0/c++
RUN autoreconf -i
RUN CXXFLAGS=-fpermissive ./configure
RUN make -j
RUN make install
RUN make clean

# Cleanup
WORKDIR /
RUN rm  /tmp/capnproto.tar.gz
RUN rm -rf /tmp/capnproto-0.4.0
RUN rm -rf /tmp/qemu
