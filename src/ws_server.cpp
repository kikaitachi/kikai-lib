module;

#include <stdio.h>

export module ws_server;

export class WebSocketServer {
 public:
  WebSocketServer() {
    printf("WebSocketServer\n");
  }
};
