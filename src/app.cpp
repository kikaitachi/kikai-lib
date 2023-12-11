module;

import comms;
import io_loop;
import logger;
import websocket;

export module app;

export namespace App {

class Application {
 public:
  Application(int port = 3000) {
    io_loop = new IOLoop();
    ws_server = new WebSocket::Server(*io_loop, port);
    // TODO: add logger comms item
  }

  void start() {
    io_loop->start();
  }

 private:
  IOLoop* io_loop;
  WebSocket::Server* ws_server;
};

}
