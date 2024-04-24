mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Test ..
mv ./compile_commands.json ../compile_commands.json
make && ./symbolic
cd ..
