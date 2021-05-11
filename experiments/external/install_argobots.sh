#!/usr/bin/bash

# exit when any command fails
set -e

wget https://github.com/pmodels/argobots/releases/download/v1.1/argobots-1.1.tar.gz
tar xvf argobots-1.1.tar.gz
cd argobots-1.1
./configure --prefix=$(realpath ../argobots-1.1-install) --enable-perf-opt --enable-affinity --disable-checks | tee config.log 2>&1
make VERBOSE=1 -j | tee make.log 2>&1
make VERBOSE=1 -j install | tee install.log 2>&1
mv *.log ../argobots-1.1-install
cd ..
rm argobots-1.1.tar.gz
rm -r argobots-1.1