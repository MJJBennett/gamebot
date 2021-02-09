#!/bin/bash

# Get the script directory relative to the current directory
script_dir=$(dirname "$0")

# Move to that directory so we can reliably move to the correct build directory
cd $script_dir/..

# Make and move to the output directory, ignore errors
mkdir output >/dev/null 2>&1
cd output

# We need OpenSSL to run this.
echo "Running cmake:"
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl

echo "Building the project:"
make

echo "Saving compile commands:"
cp compile_commands.json ..

echo "Fixing compile commands:"
cd ..
./scripts/fix-compile-commands.py include src auth
