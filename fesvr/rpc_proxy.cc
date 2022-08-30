#include "rpc_proxy.h"

#include "htif.h"
#include "byteorder.h"
#include "memif.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/poll.h>

rpc_proxy_t::rpc_proxy_t(htif_t* htif)
  : htif(htif), tohost_sd(-1)
{
  register_command(0, std::bind(&rpc_proxy_t::handle_forwarding, this, std::placeholders::_1), "rpc_forwarding");

  tohost_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (tohost_sd == -1) {
    throw std::runtime_error("rpc_proxy could not acquire new socket");
  }

  struct sockaddr_in host_addr;
  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(RPC_PROXY_PORT);
  if (inet_aton("127.0.0.1", &host_addr.sin_addr) == 0) {
    throw std::runtime_error("rpc_proxy could not convert the host address");
  }

  if (connect(tohost_sd, (struct sockaddr*)&host_addr, sizeof(host_addr)) < 0) {
    throw std::runtime_error("rpc_proxy could not connect to the host rpc server");
  }
}

rpc_proxy_t::~rpc_proxy_t()
{
  if (tohost_sd != -1) {
    close(tohost_sd);
  }
}

void rpc_proxy_t::handle_forwarding(command_t cmd)
{
  memif_t* memif = &htif->memif();
  // cmd.payload() value must contain only buffer address
  addr_t base_addr = cmd.payload();
  addr_t offset = base_addr;
  // The buffer is expected to have data in the following format:
  // [ tohost_payload_size (8B) | tohost_payload_addr | fromhost_payload_size (8B) | fromhost_payload_addr ]
  uint64_t tohost_payload_size;
  addr_t tohost_payload_addr;
  uint64_t fromhost_payload_size;
  addr_t fromhost_payload_addr;

  memif->read(offset, sizeof(tohost_payload_size), &tohost_payload_size);
  offset += sizeof(tohost_payload_size);
  memif->read(offset, sizeof(tohost_payload_addr), &tohost_payload_addr);
  offset += sizeof(tohost_payload_addr);
  memif->read(offset, sizeof(fromhost_payload_size), &fromhost_payload_size);
  offset += sizeof(fromhost_payload_size);
  memif->read(offset, sizeof(fromhost_payload_addr), &fromhost_payload_addr);

  uint8_t* payload = new uint8_t[tohost_payload_size];
  memif->read(tohost_payload_addr, tohost_payload_size, payload);
  send_to_host(payload, tohost_payload_size);
  delete[] payload;

  payload = new uint8_t[fromhost_payload_size];
  recv_from_host(payload, fromhost_payload_size);
  memif->write(fromhost_payload_addr, fromhost_payload_size, payload);
  delete[] payload;

  cmd.respond(1);
}

int rpc_proxy_t::send_to_host(const void* src, const size_t len)
{
  send(tohost_sd, &len, sizeof(len), 0);
  wait_signal(SIGACK);

  size_t total_size = 0;
  do {
    ssize_t sent_size = 0;
    sent_size = send(tohost_sd, (char*)src + total_size, len - total_size, 0);
    if (sent_size == -1) {
      throw std::runtime_error("rpc_proxy could not send data to host");
    }
    total_size += sent_size;
  } while (total_size != len);
  wait_signal(SIGACK);
  return 0;
}

int rpc_proxy_t::wait_signal(proxy_signals_t sig)
{
  proxy_signals_t recvd_signal;
  pollfd poll_desc = { .fd = tohost_sd, .events = POLLIN };
  while (poll(&poll_desc, 1, -1) == 1) { // polling only one socket
    recv(tohost_sd, &recvd_signal, sizeof(recvd_signal), 0);
    if (recvd_signal == sig) {
      return 0;
    }
  }
  return -1;
}

int rpc_proxy_t::recv_from_host(void* dst, const size_t len)
{
  wait_signal(SIGRDY);

  size_t total_size = 0;
  do {
    ssize_t recvd_size = recv(tohost_sd, (char*)dst + total_size, len - total_size, 0);
    if (recvd_size == -1) {
      throw std::runtime_error("rpc_proxy could not receive data from host");
    }
    total_size += recvd_size;
  } while (total_size != len);
  return 0;
}