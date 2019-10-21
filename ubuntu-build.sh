cd output >/dev/null 2>&1
#export BOOST_ROOT=/home/michael/install/boost_1_71_0/boost/
#export BOOST_ROOT=/home/michael/install/boost_1_71_0/boost/
#export BOOST_ROOT=/home/michael/install/boost_1_71_0/boost/
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 -DCMAKE_C_COMPILER=/usr/bin/gcc-8 -DBOOST_ROOT="/home/michael/install/boost_1_71_0" -DBOOST_INCLUDEDIR=/home/michael/install/boost_1_71_0 -DBoost_DEBUG=ON -DBOOST_LIBRARYDIR=/home/michael/install/boost_1_71_0/stage/lib/
# rm ../compile_commands.json
# ln -s compile_commands.json ../compile_commands.json
cp compile_commands.json ..
cd ..
echo "Fixing compile commands..."
./fix-compile-commands.py include src auth
