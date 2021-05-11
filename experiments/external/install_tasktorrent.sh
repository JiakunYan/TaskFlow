#!/usr/bin/bash

# exit when any command fails
set -e

export EIGEN3_ROOT=$(realpath "./eigen-3.3.9-install/include")
if [[ ! -d ./tasktorrent ]]; then
  git clone --depth 1 https://github.com/leopoldcambier/tasktorrent.git
fi
mkdir -p tasktorrent-build
cd tasktorrent-build
# build with MPI-memory version for now
# The MPI version and shared-memory version of tasktorrent are basically the same
# The only difference is the completion detection mechanism for taskpool
cmake -DCMAKE_INSTALL_PREFIX=$(realpath ../tasktorrent-install) \
      -DCMAKE_BUILD_TYPE=Release \
      -DTTOR_SHARED=OFF \
      ../tasktorrent | tee cmake.log 2>&1
make VERBOSE=1 -j | tee make.log 2>&1
# tasktorrent doesn't have the install step configured
#make VERBOSE=1 -j install | tee install.log 2>&1
cd ..
# manually "install"
mkdir -p tasktorrent-install/include
cp tasktorrent/tasktorrent/tasktorrent.hpp tasktorrent-install/include
cp -r tasktorrent/tasktorrent/src tasktorrent-install/include
mkdir -p tasktorrent-install/lib
cp tasktorrent-build/tasktorrent/src/libTaskTorrent.a tasktorrent-install/lib
cp tasktorrent-build/*.log tasktorrent-install
rm -rf tasktorrent tasktorrent-build