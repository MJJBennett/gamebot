cd output >/dev/null 2>&1
cmake .. -DCMAKE_CXX_COMPILER=/usr/local/bin/g++ -DCMAKE_C_COMPILER=/usr/local/bin/gcc-8
# rm ../compile_commands.json
# ln -s compile_commands.json ../compile_commands.json
cp compile_commands.json ..
cd ..
echo "Fixing compile commands..."
./fix-compile-commands.py include src auth
