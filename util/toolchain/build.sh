#!/bin/bash

mkdir build-binutils
mkdir build-gcc
mkdir build-newlib
mkdir build-gmp
mkdir build-mpfr
mkdir build-mpc

PREFIX=${PREFIX:=/usr/local}
NCPU=${NCPU:=1}
TARGET=x86_64-pc-ebbrt

cd build-gmp
../gmp/configure --prefix=$PREFIX --enable-cxx --disable-shared || exit
make -j$NCPU || exit
make install || exit
cd ..

cd build-mpfr
../mpfr/configure --prefix=$PREFIX --with-gmp=$PREFIX --disable-shared || exit
make -j$NCPU || exit
make install || exit
cd ..

cd build-mpc
../mpc/configure --target=$TARGET --prefix=$PREFIX --with-gmp=$PREFIX \
    --with-mpfr=$PREFIX --disable-shared || exit
make -j$NCPU || exit
make install || exit
cd ..

cd build-binutils
../binutils/configure --target=$TARGET --prefix=$PREFIX --with-gmp=$PREFIX \
    --with-mpfr=$PREFIX --with-mpc=$PREFIX --disable-werror --enable-plugins \
    || exit
make -j$NCPU || exit
make install || exit
cd ..

cd gcc/libstdc++-v3
autoconf2.64 || exit
cd ../..

cd gcc/libgcc
autoconf2.64 || exit
cd ../..

cd gcc/gcc
autoconf2.64 || exit
cd ../..

cd build-gcc
../gcc/configure --target=$TARGET --prefix=$PREFIX --enable-languages=c,c++ \
    --disable-libssp --with-gmp=$PREFIX --with-mpfr=$PREFIX --with-mpc=$PREFIX \
    --without-headers --disable-nls --with-newlib --enable-threads=ebbrt || exit
make -j$NCPU all-gcc || exit
make install-gcc || exit
cd ..

cd newlib/newlib/libc/sys
autoconf2.64 || exit
cd ebbrt
autoreconf || exit
cd ../../../../..

PATH=$PATH:$PREFIX/bin

cd build-newlib
../newlib/configure --target=$TARGET --prefix=$PREFIX \
    --with-gmp=$PREFIX --with-mpfr=$PREFIX --with-mpc=$PREFIX \
    --enable-newlib-hw-fp || exit
make -j$NCPU || exit
make install || exit
cd ..

cd build-gcc
make -j$NCPU all || exit
make install || exit
cd ..
