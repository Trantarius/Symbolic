# Symbolic

## Usage

```
name: x + y * z
```

Creates an expression and gives it a name. Named expressions are put in a global workspace and can be referenced anywhere as if it were the expression itself (TODO). 

```
$command arg1 arg2 ...
```

Executes a command named 'command', with args (arg1, arg2). Use ```$help``` for a list of commands.

## Building

```
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE={Release|Debug|Test}] ..
make [install]
```

It might be necessary to use ```jupyter kernelspec install path/to/kernel.json/dir``` to get jupyter notebook / lab to recognize the kernel. Test build type will make the binary run all tests; ie, it will NOT work as a jupyter kernel. Add ```-DCMAKE_INSTALL_PREFIX=~/.local``` to install to local directories instead of system wide. See build.sh and test.sh for examples.

#### Dependencies
Depends on [Boost::regex](https://www.boost.org/doc/libs/1_85_0/libs/regex/doc/html/index.html). Uses [```xeus```](https://github.com/jupyter-xeus/xeus) for jupyter kernel interface. [```xeus```](https://github.com/jupyter-xeus/xeus) requires [```libzmq```](https://github.com/zeromq/libzmq), [```cppzmq```](https://github.com/zeromq/cppzmq), [```nlohmann-json```](https://github.com/nlohmann/json), [```xtl```](https://github.com/xtensor-stack/xtl), and [```xeus-zmq```](https://github.com/jupyter-xeus/xeus-zmq).
