#!/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../include/scripts.sh

task="micro_deps.slurm"
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
for i in $(eval echo {1..${1:-1}}); do
  python3 ../micro_deps_run.py -p ${exe_path} | tee output.${i}.log
done
cd ..