#include "monitoringwindow.h"

int MonitoringWindow::init() {
  return 0;
}

int MonitoringWindow::show() {
  ImGui::Begin("Monitoring");
  {
    for(const auto& metric: Monitoring::getBuffers()) {
      void *buf=metric.second.getData();
      std::string id=metric.first;
      ImGui::PlotLines(id.c_str(),&MonitoringBuffer::values_getter,buf,MonitoringBuffer::elem,0,nullptr,FLT_MAX,FLT_MAX,ImVec2(0,100));
    }
  }
  ImGui::End();
  return 0;
}
