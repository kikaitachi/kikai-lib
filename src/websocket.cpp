module;

import comms;
import io_loop;
import logger;
import sha1;

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

export module websocket;

const char* web_socket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

export namespace WebSocket {

class Server {
 public:
  Server(IOLoop& io_loop, int port) : io_loop(io_loop) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
      logger::last("Failed to create WebSocket server socket");
      return;
    }
    int sock_option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &sock_option,
                   sizeof(sock_option)) == -1) {
      logger::last("Can't enable SO_REUSEADDR option for WebSocket server socket");
      return;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ==
        -1) {
      logger::last("Failed to bind WebSocket server socket");
      return;
    }
    if (listen(server_fd, 10) == -1) {
      logger::last("Failed to listen to WebSocket server socket");
      return;
    }
    io_loop.add_handler(server_fd, IOLoop::read, [&](int fd, uint32_t events) {
      accept_connection(fd);
    });
    logger::info("WebSocket server started on port %d\n", port);
  }
 private:
  IOLoop& io_loop;

  void accept_connection(int fd) {
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(fd, &addr, &addr_len);
    if (client_fd == -1) {
      logger::last("Failed to accept connection on server socket %d", fd);
      return;
    }
    logger::info("New connection with address length: %d", addr_len);
    // TODO: implement
  }
};

}
