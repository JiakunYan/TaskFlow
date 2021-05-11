import re
import glob
import numpy as np
import ast
import pandas as pd
import os,sys
sys.path.append("../../include")
from draw_simple import *
import json

name = "cholesky"
input_path = "run/slurm_output.*"
output_path = "draw/"
edge_data = {
    "format": "(.+) nranks: (\d+) \[.+\] nthreads: (\d+) N: (\d+) n: (\d+) time: (\S+)(?:\s(.+))?",
    "label": ["task", "nranks", "nthreads", "N", "n", "time", "extra"]
}
all_labels = ["task", "ncores", "nranks", "nthreads", "N", "n", "time", "extra"]

def get_typed_value(value):
    if value == '-nan':
        return np.nan
    try:
        typed_value = ast.literal_eval(value)
    except:
        typed_value = value
    return typed_value

if __name__ == "__main__":
    filenames = glob.glob(input_path)
    lines = []
    for filename in filenames:
        with open(filename) as f:
            lines.extend(f.readlines())

    df = pd.DataFrame(columns=all_labels)
    state = "init"
    current_entry = dict()
    for line in lines:
        line = line.strip()
        m = re.match(edge_data["format"], line)
        if m:
            current_data = [get_typed_value(x) for x in m.groups()]
            current_label = edge_data["label"]
            if not current_data[-1]:
                # no extra
                current_data[-1] = ""
            for label, data in zip(current_label, current_data):
                current_entry[label] = data
            df = df.append(current_entry, ignore_index=True, sort=True)
            continue
    if df.shape[0] == 0:
        print("Error! Get 0 entries!")
        exit(1)
    else:
        print("get {} entries".format(df.shape[0]))
    df = df.reindex(columns=all_labels)
    df["nthreads"] = df["nthreads"] + 1
    df["ncores"] = df["nthreads"] * df["nranks"]
    df = df.sort_values(["ncores", "n"])
    df.to_csv(os.path.join(output_path, "{}.csv".format(name)))

    # df = pd.read_csv("draw/basic.csv")
    df1 = df[df.apply(lambda row: row["N"] == 128, axis=1)]
    draw_config1 = {
        "name": "strong scaling",
        "x_key": "ncores",
        "y_key": "time",
        "tag_keys": ["task", "extra"],
        "output": "draw/",
    }
    draw_tags(draw_config1, df=df1)

    df2 = df[df.apply(lambda row: row["N"] == int(128*np.sqrt(row["ncores"]/128)), axis=1)]
    draw_config1 = {
        "name": "weak scaling",
        "x_key": "ncores",
        "y_key": "time",
        "tag_keys": ["task", "extra"],
        "output": "draw/",
    }
    draw_tags(draw_config1, df=df2, drawError=False)

    df2 = df[df.apply(lambda row: row["nranks"] == 4, axis=1)]
    draw_config1 = {
        "name": "granularity",
        "x_key": "n",
        "y_key": "time",
        "tag_keys": ["task"],
        "output": "draw/",
    }
    draw_tags(draw_config1, df=df2, drawError=False)