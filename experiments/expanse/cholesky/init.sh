#!/usr/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../../include/scripts.sh

# get the TaskFlow source path via environment variable or default value
TF_SOURCE_PATH=$(realpath "${TF_SOURCE_PATH:-../../../}")
ARGOBOTS_INSTALL_PATH=$(realpath "../../external/argobots-1.1-install")
export ARGOBOTS_ROOT=${ARGOBOTS_INSTALL_PATH}
EIGEN3_INSTALL_PATH=$(realpath "../../external/eigen-3.3.9-install")
export EIGEN3_ROOT=${EIGEN3_INSTALL_PATH}

if [[ -f "${TF_SOURCE_PATH}/src/tf.hpp" ]]; then
  echo "Found TaskFlow at ${TF_SOURCE_PATH}"
else
  echo "Did not find TaskFlow at ${TF_SOURCE_PATH}!"
  exit 1
fi

# create the ./init directory
mkdir_s ./init
# move to ./init directory
cd init

# setup module environment
module purge
module load DefaultModules
module load gcc
module load openmpi
module load cmake
export CC=gcc
export CXX=g++

# record build status
record_env

mkdir -p log
mv *.log log

# build TF
mkdir -p build
cd build
echo "Running cmake..."
TF_INSTALL_PATH=$(realpath "../install")
cmake -DCMAKE_INSTALL_PREFIX=${TF_INSTALL_PATH} \
      -DCMAKE_BUILD_TYPE=Release \
      -DTF_USE_LIBCUCKOO=ON \
      -L \
      ${TF_SOURCE_PATH} | tee init-cmake.log 2>&1 || { echo "cmake error!"; exit 1; }
cmake -LAH . >> init-cmake.log
echo "Running make..."
make VERBOSE=1 -j | tee init-make.log 2>&1 || { echo "make error!"; exit 1; }
#echo "Installing taskFlow to ${TF_INSTALL_PATH}"
#mkdir -p ${TF_INSTALL_PATH}
#make install > init-install.log 2>&1 || { echo "install error!"; exit 1; }
mv *.log ../log
cd ..

# build TF with traditional hashtable
mkdir -p build-oht # old hash table
cd build-oht
echo "Running cmake..."
TF_INSTALL_PATH=$(realpath "../install")
cmake -DCMAKE_INSTALL_PREFIX=${TF_INSTALL_PATH} \
      -DCMAKE_BUILD_TYPE=Release \
      -DTF_USE_LIBCUCKOO=OFF \
      -L \
      ${TF_SOURCE_PATH} | tee init-cmake.log 2>&1 || { echo "cmake error!"; exit 1; }
cmake -LAH . >> init-cmake.log
echo "Running make..."
make VERBOSE=1 -j | tee init-make.log 2>&1 || { echo "make error!"; exit 1; }
#echo "Installing taskFlow to ${TF_INSTALL_PATH}"
#mkdir -p ${TF_INSTALL_PATH}
#make install > init-install.log 2>&1 || { echo "install error!"; exit 1; }
cd ..