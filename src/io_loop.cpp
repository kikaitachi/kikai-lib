module;

#include <signal.h>
#include <sys/epoll.h>

#include <cstring>
#include <functional>
#include <map>

import logger;

export module io_loop;

#define MAX_EPOLL_EVENTS 4

export class IOLoop {
 private:
  int epoll_fd;
  std::map<int, std::function<void(int, uint32_t)>> fd_to_handler;

 public:
  static const uint32_t read = EPOLLIN;
  static const uint32_t write = EPOLLOUT;
  static const uint32_t one_shot = EPOLLONESHOT;
  static const uint32_t edge_triggered = EPOLLET;
  static const uint32_t closed = EPOLLRDHUP | EPOLLHUP;
  static const uint32_t error = EPOLLERR;

  bool terminated = false;

  IOLoop() {
    // Disable SIGPIPE signal, which is raised when calling sendfile
    // with closed socket and by default terminates the process
    signal(SIGPIPE, SIG_IGN);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      logger::last("Can't create epoll descriptor");
    }
    // Add handler for closing stdin which will be a symptom of terminating ssh session
    add_handler(0, closed | edge_triggered, [&](int fd, uint32_t events) {
      terminated = true;
    });
  }

  bool add_handler(int fd, uint32_t events, std::function<void(int, uint32_t)> handler) {
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
      logger::last("Failed to add descriptor %d to epoll descriptor %d for events %d",
                   fd, epoll_fd, events);
      return false;
    }
    fd_to_handler[fd] = handler;
    return true;
  }

  bool update_handler(int fd, uint32_t events) {
    struct epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
      logger::last("Failed to update descriptor %d to epoll descriptor %d for events %d",
                   fd, epoll_fd, events);
      return false;
    }
    return true;
  }

  void remove_handler(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
      logger::last("Failed to remove descriptor %d from epoll descriptor %d",
                   fd, epoll_fd);
    }
    fd_to_handler.erase(fd);
  }

  /**
   * Start polling for events.
   * This method never returns.
   */
  void start() {
    for (;;) {
      struct epoll_event events[MAX_EPOLL_EVENTS];
      int result = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
      for (int i = 0; i < result; i++) {
        int fd = events[i].data.fd;
        auto handler = fd_to_handler.find(fd);
        if (handler == fd_to_handler.end()) {
          logger::error("Got epoll event for descriptor %d which doesn't have handler", fd);
        } else {
          handler->second(fd, events[i].events);
        }
      }
    }
  }
};
