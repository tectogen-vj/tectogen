#include "logs.h"

#include "app.h"

#include <GL/glew.h>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>

extern void printGLErrors() {
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR)
  {
    Logs::get().logf(logSeverity_Err, "GL", "OpenGL error: %i", err);
  }
}

LogEntry::LogEntry(const logSeverity severity, const char* emitter, const std::string& message, const size_t timestamp)
    : severity(severity), emitter(emitter), message(message), timestamp(timestamp) {}

void Logs::add(const LogEntry& e) {
  if(stateful) {
    entries.push_back(e);
    if (filter != logSeverity_Any && e.severity & filter) {
      filterCache.push_back(&entries.back());
    }
  }
}

void Logs::log(const logSeverity severity, const char* emitter, const std::string& message) {
  if(severity==logSeverity_Err) {
    volatile int i=0;
    if(raiseOnError) {
#ifndef WIN32
      raise(SIGTRAP);
#endif
    }
  }
  auto now=std::chrono::high_resolution_clock::now();
  size_t timestamp=std::chrono::duration_cast<std::chrono::milliseconds>(now-App::get().startTime).count();
  std::lock_guard lg(logMutex);
  //if (severity != logSeverity_Debug) {
  std::cout << timestamp << " "<< emitter << ": " << message << std::endl;
  //}
  std::size_t current, previous = 0;
  current = message.find('\n');
  while (current != std::string::npos) {
    add(LogEntry(severity, emitter, message.substr(previous, current - previous), timestamp));
    previous = current + 1;
    current = message.find('\n', previous);
  }
  add(LogEntry(severity, emitter, message.substr(previous, current - previous), timestamp));
}

void Logs::logf(logSeverity severity, const char* emitter, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  va_list copy;
  va_copy(copy, args);
  int len = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);
  if (len >= 0) {
    std::string s(std::size_t(len) + 1, '\0');
    std::vsnprintf(&s[0], s.size(), fmt, args);
    log(severity, emitter, s);
  } else {
    log(severity, emitter, fmt);
    log(logSeverity_Warn, "logs", "Unable to format error message!");
  }
  va_end(args);
}

Logs& Logs::get() {
  static Logs instance;
  return instance;
}

int Logs::length() {
  if (filter == logSeverity_Any) {
    std::lock_guard lg(logMutex);
    return (int)entries.size();
  } else {
    std::lock_guard lg(logMutex);
    return (int)filterCache.size();
  }
}

Logs::Logs() : filter(logSeverity_Any) {}

Logs::~Logs() {
  log(logSeverity_Info, "Logs", "Byebye");
  stateful=false;
}

void Logs::setFilter(logSeverity filter) {
  if (this->filter == filter) {
    return;
  }
  std::lock_guard lg(logMutex);
  this->filter = filter;
  if (filter != logSeverity_Any) {
    filterCache.clear();
    for (unsigned long i = 0; i < entries.size(); i++) {
      if (entries[i].severity & filter) {
        filterCache.push_back(&entries[i]);
      }
    }
  }
}

LogEntry& Logs::getline(unsigned long idx) {
  if (filter == logSeverity_Any) {
    std::lock_guard lg(logMutex);
    return entries[idx];
  } else {
    std::lock_guard lg(logMutex);
    return *filterCache[idx];
  }
}

void Logs::setRaise(bool raiseOnError) {
  this->raiseOnError=raiseOnError;
}
