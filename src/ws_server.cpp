module;

import comms;
import logger;
import sha1;

export module ws_server;

export class WebSocketServer {
 public:
  WebSocketServer(int port) {
    log::info("WebSocket server started on port %d\n", port);
  }
};
