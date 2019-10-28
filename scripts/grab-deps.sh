#!/bin/bash

# Get the script directory relative to the current directory
script_dir=$(dirname "$0")

# Move to that directory so we can reliably move to the correct build directory
cd $script_dir/..

# Clone nlohmann/json into dependencies folder
git clone https://github.com/nlohmann/json.git deps/nlohmann_json
