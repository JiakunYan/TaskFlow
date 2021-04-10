#!/bin/bash
#SBATCH --partition=cs
#SBATCH --time=10:00
#SBATCH --nodes=1
#SBATCH --job-name=micro_deps
#SBATCH --output=slurm_output.%x-o%j
#SBATCH --error=slurm_error.%x-o%j

path_to_py=${1:-./}
path_to_exe=${2:-./init/build/benchmarks}
python3 ${path_to_py}/micro_deps_bench.py -p ${path_to_exe}
