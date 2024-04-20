mkdir build
cd build
cmake ..
make
make install
cd ..
mv ./build/compile_commands.json ./compile_commands.json
