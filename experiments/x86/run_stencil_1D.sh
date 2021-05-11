#!/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../include/scripts.sh

task="stencil_1D_shared.slurm"
exe_path=$(realpath "${exe_path:-init/build/benchmarks}")

if [[ -d "${exe_path}" ]]; then
  echo "Run TaskFlow benchmarks at ${exe_path}"
else
  echo "Did not find benchmarks at ${exe_path}!"
  exit 1
fi

# create the ./run directory
mkdir_s ./run

cd run
for i in $(eval echo {1..${1:-7}}); do
  python3 ../stencil_1D_run.py -p ${exe_path} | tee output.${i}.log
done
cd ..