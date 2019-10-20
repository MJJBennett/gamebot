#!/bin/bash

# Get the script directory relative to the current directory
script_dir=$(dirname "$0")

# Move to that directory so we can reliably move to the correct build directory
cd $script_dir

# Make and move to the output directory, ignore errors
mkdir output >/dev/null 2>&1
cd output

# Run makefile generator, with testing enabled
cmake .. -DCMAKE_CXX_COMPILER=/usr/local/bin/g++ -DCMAKE_C_COMPILER=/usr/local/bin/gcc-8 -DQBTEST=True
cmake_success=$?

# Update compile commands
#########################
cp compile_commands.json ..
cd ..
echo "Fixing compile commands..."
./fix-compile-commands.py include src auth test
#########################

# Now build tests
if [ $cmake_success -ne 0 ]; then
    echo "CMake failed, fix errors there first."
    return
fi

cd output
make all
if [ $? -ne 0 ]; then
    echo "Not running tests due to Make failure."
    return
fi

# Run tests
./test/QueueBotTests