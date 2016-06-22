FROM fedora
RUN yum -y install git python-pip findutils && \
    yum clean all && \
    mkdir /app && \
    cd /app && \
    git clone -b cgwalters-master https://github.com/cgwalters/homu.git && \
    pip3 install ./homu && \
    find /usr/lib/python* -name git_helper.py -exec chmod a+x {} ';' && \
    groupadd -g 5000 homu && \
    useradd -u 5000 -g homu homu
USER homu
ADD known_hosts /home/homu/.ssh/known_hosts
WORKDIR /homudata
EXPOSE 54856
CMD ["/usr/bin/homu", "-v"]
