#!/usr/bin/bash

# exit when any command fails
set -e

# need hwloc
wget https://bitbucket.org/icldistcomp/parsec/get/parsec-3.0.2012.tar.gz
tar xvf parsec-3.0.2012.tar.gz
mv icldistcomp-parsec-d2ae4175f072 parsec-3.0.2012 # confusing directory name
mkdir -p parsec-3.0.2012-build
cd parsec-3.0.2012-build
# -DPARSEC_DIST_WITH_MPI=OFF will lead to compilation error
cmake -DCMAKE_INSTALL_PREFIX=$(realpath ../parsec-3.0.2012-install) \
      -DCMAKE_BUILD_TYPE=Release \
      ../parsec-3.0.2012 | tee cmake.log 2>&1
make VERBOSE=1 -j | tee make.log 2>&1
make VERBOSE=1 -j install | tee install.log 2>&1
mv *.log ../parsec-3.0.2012-install
cd ..
rm parsec-3.0.2012.tar.gz
rm -r parsec-3.0.2012 parsec-3.0.2012-build