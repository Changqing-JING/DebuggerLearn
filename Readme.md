

Trigger Sigtrap handler when using gdb
```
signal SIGTRAP
```

## Build on windows:
Need clang to build the project
```shell
mkdir build_win
cd build_win
cmake -A x64 -T clangcl ..
```