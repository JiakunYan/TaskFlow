#!/bin/bash
#SBATCH --partition=shared
#SBATCH --account=uic193
#SBATCH --time=5:00
#SBATCH --nodes=1
#SBATCH --job-name=cholesky
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

WORK_ROOT=$(realpath ${WORK_ROOT:-"./"})

weak_n=$(python -c "from math import sqrt; a = int(128*sqrt(${SLURM_NTASKS}/128)); print(a)")
# strong scaling
srun -n 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist $((${SLURM_NTASKS}-1)) 128 128
srun -n 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky $((${SLURM_NTASKS}-1)) 128 128
# weak scaling
srun -n 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist $((${SLURM_NTASKS}-1)) 128 $weak_n
srun -n 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky $((${SLURM_NTASKS}-1)) 128 $weak_n
