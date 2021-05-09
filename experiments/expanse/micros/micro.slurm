#!/bin/bash
#SBATCH --account=uic193
#SBATCH --time=5:00
#SBATCH --nodes=1
#SBATCH --job-name=micro
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

WORK_ROOT=$(realpath ${WORK_ROOT:-"./"})

nthreads=$((${SLURM_NTASKS}-1))

STIMES=(2 10 100)
for stime in "${STIMES[@]}"; do
    srun -n 1 ${WORK_ROOT}/init/build/benchmarks/micro_deps ${nthreads} $stime $((${nthreads}*4)) 1
    srun -n 1 ${WORK_ROOT}/init/build/benchmarks/ttor_micro_deps ${nthreads} $stime $((${nthreads}*4)) 1
    srun -n 1 ${WORK_ROOT}/init/build/benchmarks/parsec_micro_deps ${nthreads} $stime $((${nthreads}*4)) 1
done

NDEPS=(3 9)
for ndeps in "${NDEPS[@]}"; do
    srun -n 1 ${WORK_ROOT}/init/build/benchmarks/micro_deps ${nthreads} 100 $((${nthreads}*4)) $ndeps
    srun -n 1 ${WORK_ROOT}/init/build/benchmarks/ttor_micro_deps ${nthreads} 100 $((${nthreads}*4)) $ndeps
done