import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Run benchmarks.')
parser.add_argument("-p", "--path", type=str, help='path to the executable', default="./init/build/benchmarks")
args = parser.parse_args()

executable = os.path.join(args.path, "micro_deps")

for threads in [8, 16, 32, 64, 128]:
    # 0.5 us - 100 us
    for time in [0.5*1e-6, 1e-6, 1e-5, 1e-4]:
        for nrows in [threads*4]:
            for ndeps in [1]:
                if ndeps > nrows:
                    continue
                subprocess.run(["srun", "-n", "1", executable, str(threads), str(time), str(nrows), str(ndeps)])

for ndeps in [1, 3, 5]:
    threads = 12
    time = 1e-5
    nrows = 24
    subprocess.run(["srun", "-n", "1", executable, str(threads), str(time), str(nrows), str(ndeps)])
