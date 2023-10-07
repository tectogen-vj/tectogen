#include "audioconfigwindow.h"

#include "app.h"
#include "config.h"
#include "imconfig.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "instreammanager.h"
#include "logs.h"
#include "soundio/soundio.h"

#include <algorithm>
#include <string>

void AudioConfigWindow::openSelected()
{
  Logs::get().logf(logSeverity_Info, "Audio", "Attempt to open %s", dSelect->name);
  SoundIoInStream *new_instream = soundio_instream_create(dSelect);
  if(!new_instream) {
    Logs::get().logf(logSeverity_Err, "Audio", "Failed to open %s", dSelect->name);
    return;
  }
  // TODO: Evaluate other values for SoundIoInStream::software_latency when not using JACK https://freedesktop.org/software/pulseaudio/doxygen/structpa__buffer__attr.html
  new_instream->software_latency=((double) App::get().instreammanager.blocksize*0.5)/sampleRate;
  new_instream->format=formatSelect;
  new_instream->sample_rate=sampleRate;
  new_instream->layout=dSelect->layouts[layoutSelect];

  App::get().instreammanager.start(new_instream);
}

AudioConfigWindow::AudioConfigWindow() {}

int AudioConfigWindow::init() {
  for (int i=0; i<soundio_backend_count(nullptr); i++) {
    SoundIo* api=soundio_create();
    if(api && !soundio_connect_backend(api, soundio_get_backend(api, i))) {
      api->jack_info_callback=[](const char *msg) {
        Logs::get().log(logSeverity_Info, "JACK", msg);
      };
      api->jack_error_callback=[](const char *msg) {
        Logs::get().log(logSeverity_Err, "JACK", msg);
      };
      api->on_backend_disconnect=[](struct SoundIo *, int err) {
        Logs::get().logf(logSeverity_Err, "SoundIO", "Error %i: %s", err);
      };
      apis.push_back(api);
      if(Shaderaudio::Config::audio.backendName==soundio_backend_name(api->current_backend)) {
        soundio_flush_events(api);
        int const inputCount = soundio_input_device_count(api);
        for (int deviceID = 0; deviceID < inputCount; deviceID += 1) {
          SoundIoDevice* device = soundio_get_input_device(api, deviceID);
          if (device){
            if(Shaderaudio::Config::audio.deviceName==device->name && Shaderaudio::Config::audio.samplerate) {
              iSelect=api->current_backend;
              dSelect=device;
              sampleRate=Shaderaudio::Config::audio.samplerate;
              // TODO check if requested rate is supported, layout!
              openSelected();
            }
          }
        }
      }
    }
  }
  
  return 0;
}

const int AudioConfigWindow::preferredSampleRates[]={
  8000,
  11025,
  16000,
  22050,
  44100,
  48000,
  88200,
  96000,
  176400,
  192000,
  352800,
  384000
};

int AudioConfigWindow::show() {
  
  ImGui::Begin("Audio Configuration");
  {
    bool sampleRateChanged=false;
    bool deviceChanged=false;
    for(auto& api: apis) {
      if(ImGui::TreeNode(soundio_backend_name(api->current_backend))) {
        soundio_flush_events(api);
        const int inputCount=soundio_input_device_count(api);
        for (int deviceID = 0; deviceID < inputCount; deviceID += 1) {
          SoundIoDevice* device = soundio_get_input_device(api, deviceID);
          if (device){
            const bool isValidDevice=device->probe_error==SoundIoErrorNone; //(device->sample_rate_count > 0);
            if(ImGui::Selectable(device->name, iSelect==api->current_backend && dSelect==device, isValidDevice?0:ImGuiSelectableFlags_Disabled)) {
              iSelect=api->current_backend;
              dSelect=device;
              sampleRateChanged=true;
              deviceChanged=true;
            }
            //soundio_device_unref(device);
          }
        }
        ImGui::TreePop();
      }
    }
    if(iSelect!=SoundIoBackendNone && dSelect) {
      const float button_size = ImGui::GetFrameHeight();
      
      ImGui::PushID("label");
      ImGui::SetNextItemWidth(std::max(1.0f, ImGui::CalcItemWidth() - (button_size + ImGui::GetStyle().ItemInnerSpacing.x) * 2));
      ImGui::DragInt("", &sampleRate,1.0f, 0, 384000);
      if(ImGui::IsItemDeactivatedAfterEdit()) {
        sampleRateChanged=true;
      }
      
      ImGuiStyle& style= ImGui::GetStyle();
      const ImVec2 backup_frame_padding = style.FramePadding;
      style.FramePadding.x = style.FramePadding.y;
      ImGui::SameLine(0, style.ItemInnerSpacing.x);
      if(ImGui::Button("-", ImVec2(button_size, button_size))) {
        int i;
        for(i=0; i<IM_ARRAYSIZE(preferredSampleRates) && preferredSampleRates[i]<sampleRate; i++);
        i--;
        for(; i>=0 && !soundio_device_supports_sample_rate(dSelect, preferredSampleRates[i]); i--);
        if(i>=0) {
          sampleRate=preferredSampleRates[i];
        }
      }
      
      ImGui::SameLine(0, style.ItemInnerSpacing.x);
      if(ImGui::Button("+", ImVec2(button_size, button_size))) {
        int i;
        for(i=0; i<IM_ARRAYSIZE(preferredSampleRates) && preferredSampleRates[i]<=sampleRate; i++);
        for(; i<IM_ARRAYSIZE(preferredSampleRates) && !soundio_device_supports_sample_rate(dSelect, preferredSampleRates[i]); i++);
        if(i<IM_ARRAYSIZE(preferredSampleRates)) {
          sampleRate=preferredSampleRates[i];
        }
      }
      ImGui::SameLine(0, style.ItemInnerSpacing.x);
      ImGui::Text("Sample Rate");
      style.FramePadding = backup_frame_padding;
      
      if(sampleRateChanged) {
        sampleRate=soundio_device_nearest_sample_rate(dSelect, sampleRate);
      }
      
      ImGui::PopID();

      if(deviceChanged) {
        layoutSelect=0;
        formatSelect=dSelect->current_format;
        for (int i=0; dSelect->formats && i<dSelect->format_count; i++)
        {
          if (dSelect->formats[i] == SoundIoFormatFloat32NE) {
            formatSelect = SoundIoFormatFloat32NE;
          }
        }
      }
      
      // FIXME: wait for https://github.com/andrewrk/libsoundio/issues/267 or set all names on init
      // FIXME: "unknown" not selectable because imgui requires unique names?
      auto ifknown=[](const char* name) {
        return name?name:"unknown";
      };
      
      if (dSelect->layout_count>0 && ImGui::BeginCombo("Channel Layout", ifknown(dSelect->layouts[layoutSelect].name)))
      {
        for (int i=0; dSelect->layouts && i<dSelect->layout_count; i++)
        {
          bool is_selected = (layoutSelect==i);
          ImGui::PushID(i);
          if (ImGui::Selectable(ifknown(dSelect->layouts[i].name), is_selected)) {
            layoutSelect = i;
          }
          ImGui::PopID();
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      
      if (dSelect->format_count>0 && ImGui::BeginCombo("Stream Format", soundio_format_string(formatSelect)))
      {
        for (int i=0; dSelect->formats && i<dSelect->format_count; i++)
        {
          const bool is_selected = (formatSelect== dSelect->formats[i]);
          if (ImGui::Selectable(soundio_format_string(dSelect->formats[i]), is_selected)) {
            formatSelect = dSelect->formats[i];
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      
      
      if(ImGui::Button("Open device")) {
        
        openSelected();
      }
      ImGui::SameLine();
      if(ImGui::Button("set default")) {
        Logs::get().log(logSeverity_Warn, "UI", "STUB!");
        Shaderaudio::Config::audio.samplerate=sampleRate;
        Shaderaudio::Config::audio.backendName=soundio_backend_name(iSelect);
        Shaderaudio::Config::audio.deviceName=dSelect->name;
        ImGui::MarkIniSettingsDirty();
      }
    }
  }
  ImGui::End();
  return 0;
}

