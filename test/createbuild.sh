#!/bin/bash
# my script for setting up build trees for both bare metal
# and linux libraries

#set -x

dir=$(cd $1 && echo $PWD)
echo "source directory is: " $dir
# fixme: make flags optionally an argument
flags="-O0 -g"

if [[ -z $dir ]]
then
    dir=$(basename $(pwd))
    cd ..
fi

if [[ ! -a $dir/configure.ac ]]
then
  echo "ERROR: $dir does not seem to be a src dir"
  exit -1
fi

(cd $dir; autoreconf --install)


if [[ ! -a $dir/configure ]]
then
  echo "ERROR: $dir/configure not found"
  exit -1
fi


builddir=$dir/../build
linuxdir=$builddir/linux
baredir=$builddir/bare

echo "-----Creating and configuring linux build directory----"
mkdir -p $linuxdir
cd $linuxdir; 
CFLAGS=$flags CXXFLAGS=$flags ; $dir/configure 
if [ "$?" -ne "0" ]; then 
	echo " ERROR configure of linux failed\n"
	exit -1;
fi
echo "-----making linux build----"
make || exit ;

echo "-----Creating and configuring bare build directory----"
hash x86_64-pc-ebbrt-c++ >2 /dev/null || exit
mkdir -p $baredir
cd baredir;
CFLAGS=$flags CXXFLAGS=$flags ; $dir/configure --host=x86_64-pc-ebbrt --enable-baremetal host_alias=x86_64-pc-ebbrt 
if [ "$?" -ne "0" ]; then 
	echo " ERROR configure of linux failed\n"
	exit -1;
fi
echo "-----making bare metal build----"
make || exit ;
