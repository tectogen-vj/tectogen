#pragma once

#include <map>
#include <mutex>
#include <string>
#include <array>

// forward decl
class Monitoring;

class MonitoringBuffer {
public:
  static constexpr size_t elem=128;
protected:
  friend class Monitoring;
  std::array<float, elem> buffer;
  size_t sample_count=0;
  void consume(float v) {
    sample_count++;
    buffer[sample_count%elem]=v;
  }
  float read(size_t idx) {
    return buffer[(sample_count-idx)%elem];
  }
public:
  static float values_getter(void* data, int idx) {
    auto* self=(MonitoringBuffer*)data;
    return self->read(idx);
  };
  void* getData() const {
    return (void*)this;
  }
};

class Monitoring {
  std::mutex mut;
  std::map<std::string, MonitoringBuffer> valueSet;
  static Monitoring& get() {
    static Monitoring m;
    return m;
  }
  friend void monitor(float v, std::string id);
public:
  void insert(float v, std::string id) {
    std::lock_guard lock(mut);
    valueSet[id].consume(v);
  }
  static auto const & getBuffers() {
    return Monitoring::get().valueSet;
  }
  static void monitor(float v, std::string id) {
    Monitoring::get().insert(v, id);
  }
};
