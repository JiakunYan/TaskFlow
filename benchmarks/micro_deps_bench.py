import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Run benchmarks.')
parser.add_argument("-p", "--path", type=str, help='path to the executable', default="./")
args = parser.parse_args()

executable = os.path.join(args.path, "micro_deps")
for threads in [1, 2, 4]:
    # 0.5 us - 100 us
    for time in [0.5*1e-6, 1e-6, 1e-5, 1e-4]:
        for nrows in [threads, threads*2, threads*4]:
            for ndeps in [1, 2, 4, 8]:
                if ndeps > nrows:
                    continue
                subprocess.run([executable, str(threads), str(time), str(nrows), str(ndeps)])
