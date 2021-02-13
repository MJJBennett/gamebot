#!/bin/bash

#########
# This script is intended for building the project on unix/linux platforms.
# Please avoid adding OS-specific commands without taking other OSes into account.
#
# Troubleshooting:
# - After codebase updates, it may be necessary to do a full rebuild.
#   The easiest method of doing so is simply removing the output directory,
#   then rebuilding using this script.
# - Where possible, dependencies can be automatically installed using scripts/grab-deps.sh.
#########

# Get the script directory relative to the current directory
script_dir=$(dirname "$0")

# Move to that directory so we can reliably move to the correct build directory
cd $script_dir/..

# Make and move to the output directory, ignore errors
mkdir output >/dev/null 2>&1
cd output

# We need OpenSSL to run this.
echo "Running cmake:"
echo "$(cmake --version)"
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl

echo "Building the project:"
make

echo "Saving compile commands:"
cp compile_commands.json ..

echo "Fixing compile commands:"
cd ..
# This is a helper script for ensuring all files are listed
# in compile_commands.json (as headers are not always included)
./scripts/fix-compile-commands.py src auth
