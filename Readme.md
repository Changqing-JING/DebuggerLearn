

Trigger Sigtrap handler when using gdb
```
signal SIGTRAP
```

Allow attach on Ubuntu
```shell
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

## Run Demo on QNX
### Copy Executable to Qemu emulator
```
cd build_qnx
scp ./QNXDebugger/DebugTarget/DebugTarget root@169.254.21.103:/tmp/DebugTarget
```

## Build on windows:
Need clang to build the project
```shell
mkdir build_win
cd build_win
cmake -A x64 -T clangcl ..
```