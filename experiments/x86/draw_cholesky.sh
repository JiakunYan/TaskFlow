#!/bin/bash

# exit when any command fails
set -e
# import the the script containing common functions
source ../include/scripts.sh

task="cholesky_draw.py"

mkdir_s draw
python3 $task