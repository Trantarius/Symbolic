# Symbolic

## Usage

#### Command line

```
symbolic [-in path] [-out path] [-jupyter connection_file] <inpath> <outpath>
```

If no input path is specified, stdin/stdout is used in "interactive" mode. If no output path is specified, stdout is used. ```-jupyter``` should only be used by the jupyter frontend (TODO).

#### Symbolic

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
cmake [-DCMAKE_BUILD_TYPE=<Release/Debug>] ..
make
make install
```

#### Dependencies
Uses [```xeus```](https://github.com/jupyter-xeus/xeus) for jupyter kernel functionality. [```xeus```](https://github.com/jupyter-xeus/xeus) requires [```libzmq```](https://github.com/zeromq/libzmq), [```cppzmq```](https://github.com/zeromq/cppzmq), [```nlohmann-json```](https://github.com/nlohmann/json), [```xtl```](https://github.com/xtensor-stack/xtl), and [```xeus-zmq```](https://github.com/jupyter-xeus/xeus-zmq).
