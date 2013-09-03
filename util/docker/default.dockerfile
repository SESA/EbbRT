# HELLO WORLD  - DEFAULT DOCKFILE
# VERSION 0.0.1

FROM	      ubuntu:12.10
ADD	        myapp /myapp
ENTRYPOINT  /myapp  
