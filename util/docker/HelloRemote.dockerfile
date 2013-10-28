# HELLO REMOTE DOCKFILE 
# VERSION 0.0.1

FROM	ubuntu:12.10
ADD	  myapp /myapp
ENV 	LD_LIBRARY_PATH /usr/lib/x86_64-linux-gnu
# make sure the package repository is up to date
RUN echo "deb http://archive.ubuntu.com/ubuntu precise main universe" > /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y libpcap0.8 

ENTRYPOINT  /myapp  
