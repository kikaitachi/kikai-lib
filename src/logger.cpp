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
#include <mutex>
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
  LogEntry(std::string message) : message(message) {
     clock_gettime(CLOCK_REALTIME, &timestamp);
     level = 'R';
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

static mpscq::Queue<LogEntry> entries;

std::thread logging_thread([]() {
  for ( ; ; ) {
    entries.drain([](auto entry) {
      entry->print();
    });
    std::this_thread::sleep_for(100ms);
  }
});

export void rapid(std::string message) {
  entries.push(new LogEntry(message));
}

static level current_level = level_debug;

static std::mutex mutex;

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
  struct timespec now_timespec;
  clock_gettime(CLOCK_REALTIME, &now_timespec);
  time_t now_t = now_timespec.tv_sec;
  struct tm now_tm;
  localtime_r(&now_t, &now_tm);
  const std::lock_guard<std::mutex> lock(mutex);
  fprintf(
      stderr,
      "%02d-%02d-%02d %02d:%02d:%02d.%'09ld %c ",
      now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday,
      now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec,
      now_timespec.tv_nsec, level);
  vfprintf(stderr, (format + '\n').c_str(), argptr);
}

export void debug(const std::string format, ...) {
  if (current_level <= level_debug) {
    log('D', format);
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
