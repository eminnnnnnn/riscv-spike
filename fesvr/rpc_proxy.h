#ifndef __RPC_PROXY_H
#define __RPC_PROXY_H

#include "device.h"

class htif_t;

class rpc_proxy_t : public device_t
{
 public:
  static const int RPC_PROXY_PORT = 55555;

  rpc_proxy_t(htif_t* htif);
  ~rpc_proxy_t();

  const char* identity() { return "rpc_proxy"; }

 private:
  enum proxy_signals_t : uint8_t {
    SIGACK,
    SIGRDY
  };

  htif_t* htif;
  int tohost_sd; // client socket descriptor

  int send_to_host(const void* src, const size_t len);
  int wait_signal(proxy_signals_t sig);
  int recv_from_host(void* dst, const size_t len);

  // registered commands (according to command_func_t type):
  void handle_forwarding(command_t cmd);
};

#endif