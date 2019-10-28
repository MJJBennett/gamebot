#!/usr/bin/env python3
import json
import os

p = os.path.join(os.path.dirname(os.path.realpath(__file__)), "output/skribbl.json")

with open(p, "r") as f:
    x = json.load(f)

string = ""
for k in x:
    l = x[k]
    string+= ",".join(l) + ','

print(string)
