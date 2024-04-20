cd build && cmake .. && cd .. && cmake --build ./build && ./build/Symbolic $@
mv ./build/compile_commands.json ./compile_commands.json
