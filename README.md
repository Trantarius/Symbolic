# Symbolic

## Usage

```
name: x + y * z
```

Creates an expression and gives it a name. Named expressions are put in a global workspace and can be referenced anywhere as if it were the expression itself (TODO). 

```
$command arg1 arg2 ...
```

Executes a command named 'command', with args (arg1, arg2). Use ```$help``` for a list of commands (TODO).

## Building

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.local [-DCMAKE_BUILD_TYPE=<Release/Debug>] ..
make install
```

It might be necessary to use ```jupyter kernelspec path/to/kernel.json/dir``` to get jupyter notebook / lab to recognize the kernel.

#### Dependencies
Uses [```xeus```](https://github.com/jupyter-xeus/xeus) for jupyter kernel interface. [```xeus```](https://github.com/jupyter-xeus/xeus) requires [```libzmq```](https://github.com/zeromq/libzmq), [```cppzmq```](https://github.com/zeromq/cppzmq), [```nlohmann-json```](https://github.com/nlohmann/json), [```xtl```](https://github.com/xtensor-stack/xtl), and [```xeus-zmq```](https://github.com/jupyter-xeus/xeus-zmq).
