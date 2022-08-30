#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define RPC_PROXY_PORT 55555

// according to rpc_proxy_t::proxy_signals_t enum
enum proxy_signals_t {
  SIGACK,
  SIGRDY
};

// Communication process
//
//    HOST ---Ʌ------o-----Ʌ---------o-------------o-----o-------
//            |      |     |         |             |     |
//            |size  |ACK  |payload  |ACK          |RDY  |payload
//            |      |     |         |             |     |
//    HTIF ---o------V-----o---------V-------------V-----V-------

int main()
{
  int server_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sd == -1) {
    fprintf(stderr, "Error: Creating a new socket\n");
    exit(1);
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(RPC_PROXY_PORT);
  if (inet_aton("127.0.0.1", &server_addr.sin_addr) == 0) {
    fprintf(stderr, "Error: Address conversion\n");
    exit(1);
  }

  if (bind(server_sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    fprintf(stderr, "Error: Address binding\n");
    exit(1);
  }

  if (listen(server_sd, 1) != 0) {
    fprintf(stderr, "Error: Preparing socket to accept connections\n");
    exit(1);
  }

  struct sockaddr_in client_addr;
  socklen_t client_sock_len = sizeof(client_addr);
  int client_sd = accept(server_sd, (struct sockaddr*)&server_addr, &client_sock_len);
  if (client_sd == -1) {
    fprintf(stderr, "Error: Accepting connection\n");
    exit(1);
  }

  size_t indata_size;
  struct pollfd poll_desc = { .fd = client_sd, .events = POLLIN };
  if (poll(&poll_desc, 1, -1) == 1) {
    ssize_t recvd_size = 0;
    do {
      recvd_size = recv(client_sd, &indata_size, sizeof(indata_size), MSG_PEEK);
    } while (recvd_size != sizeof(indata_size));
    recv(client_sd, &indata_size, sizeof(indata_size), 0);
  }

  uint8_t sig = (uint8_t)SIGACK;
  if (send(client_sd, &sig, sizeof(sig), 0) == -1) {
    fprintf(stderr, "Error: Signal sending\n");
    exit(1);
  }

  char* indata = (char*)malloc(indata_size);
  if (indata == NULL) {
    fprintf(stderr, "Error: Memory allocation\n");
    exit(1);
  }
  if (poll(&poll_desc, 1, -1) == 1) {
    size_t total_size = 0;
    do {
      ssize_t recvd_size = recv(client_sd, indata + total_size, indata_size - total_size, 0);
      if (recvd_size == -1) {
        fprintf(stderr, "Error: Data receiving\n");
        exit(1);
      }
      total_size += recvd_size;
    } while (total_size != indata_size);
  }

  // send ACK again
  if (send(client_sd, &sig, sizeof(sig), 0) == -1) {
    fprintf(stderr, "Error: Signal sending\n");
    exit(1);
  }

  // the response is ready
  sig = (uint8_t)SIGRDY;
  if (send(client_sd, &sig, sizeof(sig), 0) == -1) {
    fprintf(stderr, "Error: Signal sending\n");
    exit(1);
  }

  char outdata[] = "HOST";
  size_t total_size = 0;
  do {
    ssize_t sent_size = send(client_sd, outdata + total_size, sizeof(outdata) - total_size, 0);
    if (sent_size == -1) {
      fprintf(stderr, "Error: Data sending\n");
      exit(1);
    }
    total_size += sent_size;
  } while (total_size != sizeof(outdata));

  close(client_sd);
  close(server_sd);
  free(indata);
  printf("PASSED!!!\n");

  return 0;
}
