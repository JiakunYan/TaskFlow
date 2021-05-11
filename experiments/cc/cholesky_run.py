import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Run benchmarks.')
parser.add_argument("-p", "--path", type=str, help='path to the executable', default="./init/build/benchmarks")
args = parser.parse_args()

executable = os.path.join(args.path, "cholesky_bm")

# Experiment 1 : Keep N * n = 1600, change n_threads
for threads in [1, 2, 4, 8, 12, 16]:
   subprocess.run([executable, str(threads), str(160), str(10)])

# Experiment 2 : Keep ratio n_threads / N, n, increases n_threads and N
for threads in [1, 2, 4, 8, 12, 16]:
    N = threads * 40
    subprocess.run([executable, str(threads), str(N), str(10)])

# Experiment 3 : Keep nthreads and N * n, change n
for N in [20, 40, 80, 160, 320]:
    n = 1600 / N
    threads = 4
    subprocess.run([executable, str(threads), str(N), str(n)])