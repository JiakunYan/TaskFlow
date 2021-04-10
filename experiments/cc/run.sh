#!/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../scripts.sh

task="micro_deps.slurm"
sbatch_path=$(realpath "${sbatch_path:-.}")
exe_path=$(realpath "${exe_path:-init/build/benchmarks}")

if [[ -d "${exe_path}" ]]; then
  echo "Run TaskFlow benchmarks at ${exe_path}"
else
  echo "Did not find benchmarks at ${exe_path}!"
  exit 1
fi

# create the ./run directory
mkdir_s ./run

module load python

for i in $(eval echo {1..${1:-7}}); do
  cd run
  sbatch ${sbatch_path}/${task} ${sbatch_path} ${exe_path} || { echo "sbatch error!"; exit 1; }
  cd ../
done