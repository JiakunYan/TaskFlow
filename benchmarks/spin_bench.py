import subprocess
import argparse
import os

parser = argparse.ArgumentParser(description='Run benchmarks.')
parser.add_argument("-p", "--path", type=str, help='path to the executable', default="./")
args = parser.parse_args()

executable = os.path.join(args.path, "spin")
for threads in [1, 2, 4, 8]:
    # 0.5 us - 100 us
    for time in [0.5*1e-6, 1e-6, 1e-5, 1e-4]:
        tasks = round(threads * 1.0 / time)
        subprocess.run([executable, str(threads), str(time)])
