#!/bin/bash

# Get the script directory relative to the current directory
script_dir=$(dirname "$0")

# Move to that directory so we can reliably move to the correct build directory
cd $script_dir/..

# Skribbl testing - we need to set up our test JSON data
# cp "test/test_skribbl.json" "test/test_skribbl.noedit.json"
rm "test/test_skribbl.noedit.json"

# Make and move to the output directory, ignore errors
mkdir output >/dev/null 2>&1
cd output

# Run makefile generator, with testing enabled
# cmake .. -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-9 -DCMAKE_C_COMPILER=/usr/local/bin/gcc-9 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DQBTEST=True
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DQBTEST=True
cmake_success=$?

# Update compile commands
#########################
cp compile_commands.json ..
cd ..
echo "Fixing compile commands..."
./scripts/fix-compile-commands.py include src auth test
#########################

# Now build tests
if [ $cmake_success -ne 0 ]; then
    echo "CMake failed, fix errors there first."
    exit $cmake_success
fi

cd output
make all
if [ $? -ne 0 ]; then
    echo "Not running tests due to Make failure."
    exit $cmake_success
fi

# Run tests
./test/QueueBotTests
