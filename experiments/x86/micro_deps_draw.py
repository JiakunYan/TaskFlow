import re
import glob
import numpy as np
import ast
import pandas as pd
import os,sys
sys.path.append("../include")
from draw_simple import *
import json

name = "micro_deps"
input_path = "run/output.*"
output_path = "draw/"
edge_data = {
    "format": "nthreads: (\d+)\s+ spinTime\(us\): (\S+)\s+ nrows: (\d+)\s+ ncols: \d+\s+ ndeps: (\d)\s+ time\(s\): \S+\s+  efficiency (\S+)",
    "label": ["nthreads", "spinTime", "nrows", "ndeps", "efficiency"]
}
all_labels = ["nthreads", "spinTime", "nrows", "ndeps", "efficiency"]

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
            for label, data in zip(current_label, current_data):
                current_entry[label] = data
            if "extra" in edge_data:
                for extra_datum in edge_data["extra"]:
                    current_entry[extra_datum[0]] = extra_datum[1]
            df = df.append(current_entry, ignore_index=True, sort=True)
            continue
    if df.shape[0] == 0:
        print("Error! Get 0 entries!")
        exit(1)
    else:
        print("get {} entries".format(df.shape[0]))
    df = df.reindex(columns=all_labels)
    df.to_csv(os.path.join(output_path, "{}.csv".format(name)))

    # df = pd.read_csv("draw/basic.csv")
    df1 = df[df.apply(lambda row: row["ndeps"] == 1 and
                                  row["nrows"] == row["nthreads"]*4, axis=1)]
    draw_config1 = {
        "name": "micro_deps1",
        "x_key": "spinTime",
        "y_key": "efficiency",
        "tag_keys": ["nthreads"],
        "output": "draw/",
    }
    draw_tags(draw_config1, df=df1)

    df2 = df[df.apply(lambda row: row["nthreads"] == 4 and
                                  row["spinTime"] == 10, axis=1)]
    draw_config2 = {
        "name": "micro_deps2",
        "x_key": "ndeps",
        "y_key": "efficiency",
        "tag_keys": ["nrows"],
        "output": "draw/",
    }
    draw_tags(draw_config2, df=df2)