module;

import mpscq;

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <format>
#include <string>
#include <thread>

export module logger;

using namespace std::chrono_literals;

namespace logger {

export enum level { level_debug,
                    level_info,
                    level_warn,
                    level_error };

class LogEntry {
 public:
  LogEntry(char level, std::string message)
      : level(level), message(message) {
     clock_gettime(CLOCK_REALTIME, &timestamp);
  }

  void print() {
    struct tm time;
    localtime_r(&timestamp.tv_sec, &time);
    fprintf(
        stderr,
        "%02d-%02d-%02d %02d:%02d:%02d.%'09ld %c %s\n",
        time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
        time.tm_hour, time.tm_min, time.tm_sec,
        timestamp.tv_nsec, level, message.c_str());
  }

  char level;
  timespec timestamp;
  std::string message;
};

mpscq::Queue<LogEntry> entries;

std::thread logging_thread([]() {
  for ( ; ; ) {
    entries.drain([](auto entry) {
      entry->print();
    });
    std::this_thread::sleep_for(100ms);
  }
});

static level current_level = level_debug;

export void set_level(level log_level) {
  current_level = log_level;
}

export level get_level() {
  return current_level;
}

#define log(level, message)          \
  va_list argptr;                    \
  va_start(argptr, format);          \
  log_entry(level, message, argptr); \
  va_end(argptr)

void log_entry(const char level, const std::string format, va_list argptr) {
  char message[1024];
  int len = vsnprintf(message, sizeof(message), format.c_str(), argptr);
  entries.push(new LogEntry(level, std::string(message, len)));
}

/*export void debug(const std::string format, ...) {
  if (current_level <= level_debug) {
    log('D', format);
  }
}*/

export template <typename... Args>
void debug(std::format_string<Args...> rt_fmt_str, Args&&... args) {
  if (current_level <= level_debug) {
    /*std::string str;
    auto it = std::back_inserter(str);
    std::format_to(it, rt_fmt_str, std::forward<Args>(args)...);
    entries.push(new LogEntry('D', str));*/
    entries.push(new LogEntry('D', std::format(rt_fmt_str, std::forward<Args>(args)...)));
  }
}

export void warn(std::string format, ...) {
  if (current_level <= level_warn) {
    log('W', format);
  }
}

export void error(const std::string format, ...) {
  if (current_level <= level_error) {
    log('E', format);
  }
}

export void info(const std::string format, ...) {
  if (current_level <= level_info) {
    log('I', format);
  }
}

export void last(const std::string format, ...) {
  std::string error(strerror(errno));
  log('L', format + ": " + error);
}

}  // namespace logger
