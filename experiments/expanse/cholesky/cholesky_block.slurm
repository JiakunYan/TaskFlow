#!/bin/bash
#SBATCH --partition=compute
#SBATCH --account=uic193
#SBATCH --time=5:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=cholesky
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

WORK_ROOT=$(realpath ${WORK_ROOT:-"./"})

srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 64 256
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 128
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 256 64
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 512 32
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 64 256
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 128 128
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 256 64
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 512 32
