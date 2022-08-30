#include <stdint.h>

#define SET_DEVICE(x) ((x) << 56)
#define SET_COMMAND(x) ((x) << 48)
#define SET_PAYLOAD(x) ((x) << 16 >> 16)

#define RPC_PROXY_CMD_FORWARD 0UL
#define RPC_PROXY_DEV_NUM 2UL

extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

// The buffer is expected to have data in the following format:
// [ tohost_payload_size (8B) | tohost_payload_addr | fromhost_payload_size (8B) | fromhost_payload_addr ]
typedef struct {
    uint64_t tohost_payload_size;
    uintptr_t tohost_payload_addr;
    uint64_t fromhost_payload_size;
    uintptr_t fromhost_payload_addr;
} __attribute__((packed)) payloads_descriptor_t;

int main()
{
    char tohost_payload[] = "RISCV";
    char fromhost_payload[5] = {0}; // "HOST" message expected

    volatile payloads_descriptor_t desc;
    desc.tohost_payload_size = sizeof(tohost_payload);
    desc.tohost_payload_addr = (uintptr_t)tohost_payload;
    desc.fromhost_payload_size = sizeof(fromhost_payload);
    desc.fromhost_payload_addr = (uintptr_t)fromhost_payload;

    tohost = SET_DEVICE(RPC_PROXY_DEV_NUM) | SET_COMMAND(RPC_PROXY_CMD_FORWARD) | SET_PAYLOAD((uintptr_t)&desc);
    while (tohost != 0) {}  // wait until HTIF starts processing
    while(fromhost == 0) {} // wait for a response

    // char-by-char response check (only 5 chars...not so much)
    return !(fromhost_payload[0] == 'H' && fromhost_payload[1] == 'O' && \
             fromhost_payload[2] == 'S' && fromhost_payload[3] == 'T' && fromhost_payload[4] == '\0');
}