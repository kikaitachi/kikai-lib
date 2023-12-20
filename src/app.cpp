module;

import comms;
import io_loop;
import logger;
import websocket;

#include <locale.h>

export module app;

export namespace App {

class Application {
 public:
  Application(int port = 3000) {
    setlocale(LC_NUMERIC, "");
    items = new Comms::Items();
    io_loop = new IOLoop();
    ws_server = new WebSocket::Server(*io_loop, port, *items);
    // TODO: add logger comms item
  }

  void start() {
    io_loop->start();
  }

 private:
  Comms::Items* items;
  IOLoop* io_loop;
  WebSocket::Server* ws_server;
};

}
