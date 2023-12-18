module;

import base64;
import comms;
import io_loop;
import logger;
import sha1;

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <map>
#include <string>

export module websocket;

const char* web_socket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#define OPCODE_BINARY 2
#define OPCODE_CLOSE 8

#define FINAL_FRAME 128

class WebSocketClient {
 public:
  int fd;
  char read_buffer[1024 * 8];
  int read_buffer_len = 0;
  char write_buffer[1024 * 8];
  int write_buffer_len = 0;
  bool connected = false;
};

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
    logger::info("WebSocket server started on port %d", port);
  }
 private:
  IOLoop& io_loop;
  std::map<int, WebSocketClient*> clients;

  void accept_connection(int fd) {
    sockaddr addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(fd, &addr, &addr_len);
    if (client_fd == -1) {
      logger::last("Failed to accept connection on server socket %d", fd);
      return;
    }
    char host[NI_MAXHOST];
    if (getnameinfo(&addr, addr_len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST) != 0) {
      logger::last("Failed to convert socket address to host name");
      return;
    }
    logger::info("WS(%d) new connection from %s", client_fd, host);
    WebSocketClient* client = new WebSocketClient();
    client->fd = client_fd;
    clients.insert({client_fd, client});
    io_loop.add_handler(client_fd, IOLoop::read | IOLoop::edge_triggered | IOLoop::closed, [&](int fd, uint32_t events) {
      auto iterator = clients.find(fd);
      if (iterator == clients.end()) {
        logger::error("WS(%d) received events %ld for not existing client", fd, events);
      } else {
        if (events & IOLoop::closed) {
          disconnect(iterator->second);
        } else {
          if (events & IOLoop::read) {
            handle_read(iterator->second);
          }
          if (events & IOLoop::write) {
            handle_write(iterator->second);
          }
        }
      }
    });
  }

  void disconnect(WebSocketClient* client) {
    clients.erase(client->fd);
    io_loop.remove_handler(client->fd);
    if (close(client->fd) == -1) {
      logger::last("WS(%d) failed to close", client->fd);
    } else {
      logger::info("WS(%d) closed", client->fd);
    }
    delete client;
  }

  void handle_read(WebSocketClient* client) {
    client->read_buffer_len = read(
      client->fd,
      client->read_buffer + client->read_buffer_len,
      sizeof(client->read_buffer) - client->read_buffer_len);
    if (client->read_buffer_len == -1) {
      logger::last("WS(%d) failed to read", client->fd);
      disconnect(client);
      return;
    }
    logger::info("WS(%d) read %ld bytes", client->fd, client->read_buffer_len);
    if (client->connected) {
      while (client->read_buffer_len >= 6) {
        int opcode = client->read_buffer[0] & 0x0f;
        if (opcode == OPCODE_CLOSE) {
          logger::info("WS(%d) close frame received", client->fd);
          disconnect(client);
          return;
        }
        uint64_t data_length = client->read_buffer[1] & 127;
        size_t header_length;
        if (data_length == 126) {
          header_length = 8;
          data_length = client->read_buffer[2] << 8 | client->read_buffer[3];
        } else if (data_length == 127) {
          header_length = 14;
          data_length = (uint64_t)client->read_buffer[2] << 56 |
                        (uint64_t)client->read_buffer[3] << 48 |
                        (uint64_t)client->read_buffer[4] << 40 |
                        (uint64_t)client->read_buffer[5] << 32 |
                        client->read_buffer[6] << 24 | client->read_buffer[7] << 16 |
                        client->read_buffer[7] << 8 | client->read_buffer[9];
        } else {
          header_length = 6;
        }
        size_t frame_length = data_length + header_length;
        if (client->read_buffer_len >= frame_length) {
          char mask[4];
          mask[0] = client->read_buffer[header_length - 4];
          mask[1] = client->read_buffer[header_length - 3];
          mask[2] = client->read_buffer[header_length - 2];
          mask[3] = client->read_buffer[header_length - 1];

          for (int i = 0; i < data_length; i++) {
            client->read_buffer[i + header_length] ^= mask[i % 4];
          }

          //for (auto& handler : ws_handlers) {
          //  handler(fd, data_length, client->read_buffer + header_length);
          //}
          logger::info("WS(%d): frame of %d bytes received", client->fd, data_length);

          client->read_buffer_len -= frame_length;
          memmove(client->read_buffer, client->read_buffer + frame_length, client->read_buffer_len);
        }
      }
    } else {
      std::string_view request(client->read_buffer, client->read_buffer_len);
      std::size_t start = request.find("Sec-WebSocket-Key");
      if (start != std::string_view::npos) {
        start += 19;
        std::size_t end = start + 24;
        strncpy(client->read_buffer + end, web_socket_guid, strlen(web_socket_guid));
        std::string key = std::string(request.substr(start, 24 + strlen(web_socket_guid)));
        unsigned char hash[20];
        SHA1 checksum;
        checksum.update(key);
        checksum.final(hash);
        size_t output_length;
        char* base64_encoded = base64_encode(hash, 20, &output_length);
        output_length = snprintf(
            client->write_buffer, sizeof(client->write_buffer),
            "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: "
            "Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n",
            base64_encoded);
        free(base64_encoded);
        ssize_t written = write(client->fd, client->write_buffer, output_length);
        if (written == -1) {
          logger::last("WS(%d) failed to send handshake", client->fd);
          disconnect(client);
          return;
        }
        if (written != output_length) {
          logger::error("WS(%d) incomplete handshake was send, this case is not yet implemented");
          disconnect(client);
          return;
        }
        client->read_buffer_len = 0;
        client->connected = true;
      }
    }
  }

  void handle_write(WebSocketClient* client) {
    logger::warn("WS(%d) writing is not implemented", client->fd);
  }
};

}
