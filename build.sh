mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.local ..
make install
cd ..
mv ./build/compile_commands.json ./compile_commands.json
