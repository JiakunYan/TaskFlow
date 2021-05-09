#!/bin/bash

function ceil(){
        echo "$1 $2" | awk '{a=int($1/$2); if ($1 % $2 != 0) a++; print a; }'
}

# exit when any command fails
set -e
# import the the script containing common functions
source ../../include/scripts.sh

WORK_ROOT=$(realpath .)
export WORK_ROOT=$WORK_ROOT

# create the ./run directory
mkdir_s ./run
NPROCS=(8 16 32 64 128 256 512 1024)

cd run
for i in $(eval echo {1..${1:-7}}); do
  for nprocs in "${NPROCS[@]}"; do
    if [[ ${nprocs} -lt 65 ]]; then
      sbatch -N 1 -n ${nprocs} ../cholesky.shared.slurm || { echo "sbatch error!"; exit 1; }
    else
      sbatch -N $(ceil ${nprocs} 128) ../cholesky.slurm || { echo "sbatch error!"; exit 1; }
    fi
  done
  sbatch ../cholesky_block.slurm
done
cd ../