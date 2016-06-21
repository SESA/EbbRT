FROM ubuntu:xenial
RUN apt-get update && \
    apt-get install -y \
    autoconf \
    automake \
    build-essential \
    git \
    libfdt-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    openssh-server \
    zlib1g-dev \
    dnsmasq 

RUN git clone -b pin-threads https://github.com/SESA/qemu.git /qemu
WORKDIR /qemu
RUN ./configure --target-list=x86_64-softmmu --enable-vhost-net --enable-kvm
RUN make -j
RUN make install
WORKDIR /
RUN rm -rf /qemu

# password-less root ssh login
RUN mkdir /var/run/sshd
RUN passwd root -d 
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -i 's/StrictModes yes/StrictModes no/' /etc/ssh/sshd_config
RUN sed -i 's/PermitEmptyPasswords no/PermitEmptyPasswords yes/' /etc/ssh/sshd_config
RUN sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config
RUN echo 'AuthorizedKeysFile /dev/null' >> /etc/ssh/sshd_config
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile

COPY start /root/start

EXPOSE 22
ENTRYPOINT ["/root/start"]
