#!/bin/bash
#SBATCH --partition=compute
#SBATCH --account=uic193
#SBATCH --time=5:00
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=cholesky
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

WORK_ROOT=$(realpath ${WORK_ROOT:-"./"})

weak_n=$(python -c "from math import sqrt; a = int(128*sqrt(${SLURM_NNODES})); print(a)")
# strong scaling
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 128
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 128 128
# weak scaling
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 $weak_n
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/ttor_cholesky 127 128 $weak_n

TF_CONF_sched_type=basic srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 128 | tr -d '\n' ; echo ' sched=basic'
TF_CONF_sched_type=randws srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 128 | tr -d '\n' ; echo ' sched=randws'
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build-oht/benchmarks/cholesky_dist 127 128 128 | tr -d '\n' ; echo ' old_hashtable'

TF_CONF_sched_type=basic srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 $weak_n | tr -d '\n' ; echo ' sched=basic'
TF_CONF_sched_type=randws srun --ntasks-per-node 1 ${WORK_ROOT}/init/build/benchmarks/cholesky_dist 127 128 $weak_n | tr -d '\n' ; echo ' sched=randws'
srun --ntasks-per-node 1 ${WORK_ROOT}/init/build-oht/benchmarks/cholesky_dist 127 128 $weak_n | tr -d '\n' ; echo ' old_hashtable'