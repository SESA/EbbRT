FROM fedora
RUN yum -y install git python-pip findutils && \
    yum clean all && \
    mkdir /app && \
    cd /app && \
    git clone https://github.com/servo/homu.git && \
    pip3 install ./homu && \
    find /usr/lib/python* -name git_helper.py -exec chmod a+x {} ';'
ADD known_hosts /root/.ssh/known_hosts
WORKDIR /homudata
EXPOSE 54856
CMD ["/usr/bin/homu", "-v"]
