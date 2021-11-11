#!/bin/bash
export CC=clang-10
export CXX=clang++-10
export CFLAGS='-O3 -g -fno-omit-frame-pointer'
export CXXFLAGS='-w -O3 -g -D_KINESIS'
export CONFIGURE_FLAGS=''

./autogen.sh
./configure CC="${CC}" CXX="${CXX}" CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" ${CONFIGURE_FLAGS}