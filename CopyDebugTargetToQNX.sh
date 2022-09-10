scp ./build/QNXDebugger/DebugTarget/DebugTarget root@169.254.21.103:/tmp/DebugTarget

ssh root@169.254.21.103 "qconn;pfctl -d;chmod 777 /tmp/DebugTarget;"