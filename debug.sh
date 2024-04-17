cd build && cmake .. && cd .. && cmake --build ./build && gdb ./build/Symbolic
mv ./build/compile_commands.json ./compile_commands.json
