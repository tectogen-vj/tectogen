#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#define logs_soft_assert(expr){\
  if(!(expr)) {\
    Logs::get().log(logSeverity_Warn, "assert", "Assertion failed: " #expr);\
  }}

enum logSeverity_ {
  logSeverity_Err = 1 << 0,
  logSeverity_Warn = 1 << 1,
  logSeverity_Info = 1 << 2,
  logSeverity_Debug = 1 << 3,
  logSeverity_Any = 0b1111
};

typedef int logSeverity;

struct LogEntry {
  logSeverity severity;
  const char* emitter;
  std::string message;
  size_t timestamp;
  LogEntry(const logSeverity severity, const char* emitter, const std::string& message, const size_t timestamp);
};



void printGLErrors();

class Logs {
 private:
  std::vector<LogEntry> entries;
  logSeverity filter;
  std::vector<LogEntry*> filterCache;
  void add(const LogEntry& e);
  std::mutex logMutex;
  bool raiseOnError=false;
  bool stateful=true;

 public:
  Logs();
  ~Logs();
  static Logs& get();
  void log(logSeverity severity, const char* emitter, const std::string& message);
  void logf(logSeverity severity, const char* emitter, const char* fmt, ...);
  int length();
  LogEntry& getline(unsigned long idx);
  void setFilter(logSeverity filter);
  void setRaise(bool raiseOnError);
};
