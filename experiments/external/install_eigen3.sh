#!/usr/bin/bash

# exit when any command fails
set -e

wget https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz
tar xvf eigen-3.3.9.tar.gz
mkdir -p eigen-3.3.9-build
cd eigen-3.3.9-build
cmake -DCMAKE_INSTALL_PREFIX=$(realpath ../eigen-3.3.9-install) ../eigen-3.3.9 | tee cmake.log 2>&1
make VERBOSE=1 -j | tee make.log 2>&1
make VERBOSE=1 -j install | tee install.log 2>&1
mv *.log ../eigen-3.3.9-install
cd ..
rm eigen-3.3.9.tar.gz
rm -r eigen-3.3.9 eigen-3.3.9-build