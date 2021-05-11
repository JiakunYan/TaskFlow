import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Run benchmarks.')
parser.add_argument("-p", "--path", type=str, help='path to the executable', default="./init/build/benchmarks")
args = parser.parse_args()

executable = os.path.join(args.path, "stencil_1D_shared")

# Experiment 1: change thread number
for threads in [1, 2, 4, 8, 12, 16]:
   subprocess.run([executable, str(threads), str(5000), str(5000), str(32), str(32), str(8)])

# Experiment 2: change block size 
for mb in [8, 16, 32, 64, 128, 256]:
    subprocess.run([executable, str(4), str(5000), str(5000), str(mb), str(mb), str(8)])