# RPC-PROXY TEST

## About the test

The main purpose is to test the *rpc-proxy* device that forwards data from the simulator to the host and backward. With the development of the RPC-based syscall forwarding, the data will be the arguments to system calls and the results of their execution.

## About the rpc-proxy device

The purpose of the device is only to send data and receive a response. It only knows where and how much to take from the memory of the simulator and where and how much to put without worrying about data serialization. The device communicates with host via tcp socket connected to localhost.

## Build

This test can only be built separately from the Spike build for now.
```Makefile
# if the RISCV environment variable is set
make

# or set it only for the make process
RISCV=path/to/riscv/toolchain/dir make
```

## Run

First run *riscv_host_server* in background or another process then run the target app *rpc_target_test*.
```Makefile
spike +rpc-proxy rpc_target_test
```