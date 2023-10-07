#pragma once

#include "logs.h"

#include <ctime>
#include <string>
#include <unordered_map>

class Profiler {
  Profiler(const Profiler&) = delete;
  Profiler& operator=(const Profiler&) = delete;
private:
  struct M {
    int lastest=0;
    int sum=0;
    clock_t lastPrint;
    M() {
      lastPrint=clock();
    }
  };
  static std::unordered_map<std::string,M> profs;
  clock_t start;
  const std::string id;
public:
  Profiler(const std::string& id) : id(id) {
    start=clock();
  }
  ~Profiler() {
    clock_t end = clock();
    M& m=profs[id];
    m.sum+=(end-start);
    m.lastest+=1;

    // Print results and reset counters when either
    if(
       // ten seconds past since the last print
       (clock()-m.lastPrint>CLOCKS_PER_SEC*10)
       // or there's only been one other sample in the past second
       || (clock()-m.lastPrint>CLOCKS_PER_SEC && m.lastest<3)
       ) {
      m.lastPrint=clock();
      double milis=((double)m.sum/(double)m.lastest/(double)CLOCKS_PER_SEC)*1000.0;
      Logs::get().logf(logSeverity_Debug, "Profiling", "%i %s averaged at %fms %fµs", m.lastest, id.c_str(), milis, milis*1000.0);
      m.sum=0;
      m.lastest=0;
    }
  }
};
