#!/usr/bin/env python3
import json

with open("output/skribbl.json", "r") as f:
    x = json.load(f)

string = ""
for k in x:
    l = x[k]
    string+= ",".join(l) + ','

print(string)
