RPC_TEST_CC = $(RISCV)/bin/riscv64-unknown-linux-gnu-gcc
RPC_SERVER_CC = gcc

RPC_SERVER_DEPS = rpc_server.c
RPC_SERVER_SRCS = rpc_server.c

RPC_TEST_DEPS = link.ld startup.S rpc_test.c
RPC_TEST_SRCS = startup.S rpc_test.c

.PHONY: all
all: rpc_target_test rpc_host_server

rpc_host_server: $(RPC_SERVER_DEPS)
	$(RPC_SERVER_CC) -Wall -Werror $(RPC_SERVER_SRCS) -o rpc_host_server

rpc_target_test: $(RPC_TEST_DEPS)
	$(RPC_TEST_CC) -I../riscv/ -nostartfiles -nostdlib -Wall -Werror \
     -mabi=lp64d -march=rv64imafd -mcmodel=medany -T link.ld $(RPC_TEST_SRCS) -o rpc_target_test

.PHONY: clean
clean:
	rm -f rpc_target_test rpc_host_server
