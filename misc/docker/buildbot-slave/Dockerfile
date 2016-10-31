FROM ebbrt/build-environment

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    clang-format-3.8 \
    git \
    iputils-ping \
    python-dev \
    python-pexpect \
    python-pip \
    supervisor \
    texinfo

# BuildBot
RUN pip install buildbot_slave
RUN groupadd -g 5001 buildbot
RUN useradd -u 5001 -g buildbot buildbot

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
WORKDIR /buildbotslavedata
CMD ["/usr/bin/supervisord"]
