#include "widgets.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>
#include <complex>

enum projectionFlags_ {
  projectionFlags_None        = 0,
  projectionFlags_LogX        = 1 << 0,
  projectionFlags_LogY        = 1 << 1
};

typedef int projectionFlags;

struct Projection {
  ImVec2 dataMin;
  ImVec2 dataMax;
  ImVec2 imageMin;
  ImVec2 imageMax;
  projectionFlags flags;
  Projection(ImVec2 dataMin, ImVec2 dataMax, ImVec2 imageMin, ImVec2 imageMax, projectionFlags flags)
    : dataMin(dataMin), dataMax(dataMax), imageMin(imageMin), imageMax(imageMax), flags(flags){}

  float projectX(float datum) {
    if(flags&projectionFlags_LogX) {
      return log(datum)/log(dataMax.x)*(imageMax.x-imageMin.x)+imageMin.x;
    } else {
      return (datum-dataMin.x)/(dataMax.x-dataMin.x)*(imageMax.x-imageMin.x)+imageMin.x;
    }
  }
  float projectY(float datum) {
    if(flags&projectionFlags_LogY) {
      return log(datum)/log(dataMax.y)*(imageMax.y-imageMin.y)+imageMin.y;
    } else {
      return (datum-dataMin.y)/(dataMax.y-dataMin.y)*(imageMin.y-imageMax.y)+imageMax.y;
    }
  }
  ImVec2 project(ImVec2 datum) {
    return ImVec2(projectX(datum.x), projectY(datum.y));
  }
  float unprojectX(float im) {
    return pow(dataMax.x, (im-imageMin.x)/(imageMax.x-imageMin.x));
  }
  // TODO unprojectY and meaningful log amplitude
};



inline double bin2freq(int bin, double samplerate, int nbins) {
  return samplerate*bin/(2.*(nbins-1));
}

inline double freq2bin(double freq, double samplerate, int nbins) {
  return freq*(2.*(nbins-1))/samplerate;
}

const char* notes[]={"a ", "a#", "b ", "c ", "c#", "d ", "d#", "e ", "f ", "f#","g ", "g#"};

inline auto el(std::complex<float>* arr, int idx) {
  return std::abs(arr[idx]);
}

inline auto el(float* arr, int idx) {
  return arr[idx];
}

template<typename T>
void ImGui::PlotSpectrum(const char* label, T* data, int nbins, int samplerate, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size) {
  ImGuiWindow* window = GetCurrentWindow();
  if (window->SkipItems)
    return;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID(label);

  const ImVec2 label_size = CalcTextSize(label, NULL, true);
  if (frame_size.x == 0.0f)
    frame_size.x = CalcItemWidth();
  if (frame_size.y == 0.0f)
    frame_size.y = label_size.y + (style.FramePadding.y * 2);

  const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
  const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
  const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
  ItemSize(total_bb, style.FramePadding.y);
  if (!ItemAdd(total_bb, 0, &frame_bb))
    return;

  const bool hovered = ItemHoverable(frame_bb, id);

  // Determine scale from values if not specified
  if (scale_max == FLT_MAX)
  {
    float v_max = -FLT_MAX;
    for (int i = 0; i < nbins; i++)
    {
      const float v = el(data, i);
      if (v != v) // Ignore NaN values
        continue;
      v_max = ImMax(v_max, v);
    }
    scale_max = v_max;
  }

  ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, true);
  RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

  const int values_count_min = 2;
  int values_offset=0;
  int values_count=nbins-values_offset;
  if (values_count >= values_count_min)
  {
    int res_w = ImMin((int)frame_size.x, values_count) -1;
    int item_count = values_count -1;

    const float t_step = 1.0f / (float)res_w;
    const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

    const ImU32 col_base = GetColorU32(ImGuiCol_PlotLines);
    const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotLinesHovered);
    ImColor col_lines(col_base);
    col_lines.Value.w*=0.1f;

    Projection p(ImVec2(1,scale_min),ImVec2(nbins, scale_max),inner_bb.Min, inner_bb.Max,projectionFlags_LogX);

    for(double freq=27.5; freq2bin(freq,samplerate,nbins)<nbins; freq*=2) {
      float bin=(float)freq2bin(freq,samplerate,nbins);
      window->DrawList->AddLine(ImVec2(p.projectX(bin),inner_bb.Min.y),ImVec2(p.projectX(bin),inner_bb.Max.y), col_lines);
    }

    // Tooltip on hover
    int v_hovered = -1;
    if (hovered && inner_bb.Contains(g.IO.MousePos))
    {
      const float t = p.unprojectX(g.IO.MousePos.x);
      const int v_idx = ImClamp((int)std::round(t),1,values_count);
      //IM_ASSERT(v_idx >= 0 && v_idx < values_count);

      const float v0 = el(data, (v_idx + values_offset) % values_count);



      ImVec2 pos0 = ImVec2(p.projectX(v_idx),inner_bb.Min.y);
      ImVec2 pos1 = ImVec2(p.projectX(v_idx),inner_bb.Max.y);
      window->DrawList->AddLine(pos0, pos1, col_hovered);

      {
        double freq=bin2freq((v_idx>0?v_idx:1), samplerate, nbins);
        double concert_pitch=440.0;
        double n=12.0*log2(freq/concert_pitch);
        unsigned int note=(unsigned int)round(n)%12;
        const char* notename=notes[note];
        int octave=4+(int)round(n/12);
        int cent=(int)((n-round(n))*100);
        const char* unit=" Hz";
        if(freq>1000000) {
          unit="mHz";
          freq/=1000000;
        } else if(freq>1000) {
          unit="kHz";
          freq/=1000;
        }
        SetTooltip("bin %i\n%s%#.4g%s\n%s%-2i %+3i%%\n%#4.2f",v_idx,
                   v_idx==0?"<":"",freq, unit,
                   notename, octave, cent,
                   v0);
      }
    }

    /*float v0 = data[(0 + values_offset) % values_count];
        float t0 = 0.0f;
        ImVec2 tp0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale) );                       // Point in the normalized space of our target rectangle
        float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands
        */
    //ImVec2 pos = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
    //window->DrawList->AddCircleFilled(pos,2.0f,col_hovered, 5);

    int lastpixel=-1;
    //float total=0;
    //int count=0;
    float max=-FLT_MAX;
    ImVec2 lastpoint(0,0);
    for(int n=1; n<nbins; n++) {
      int pixel=(int)p.projectX(n);
      //total+=data[n%values_count];
      //count+=1;
      if(el(data,n%values_count)>max) {
        max=el(data,n%values_count);
      }

      if(pixel>lastpixel) {
        lastpixel=pixel;
        ImVec2 point(n, max);
        max=-FLT_MAX;
        //total=0;
        //count=0;
        if(lastpoint.x>0) {
          window->DrawList->AddLine(p.project(lastpoint), p.project(point), col_base);
        }
        lastpoint=point;
      }

    }

    /*for (int n = 0; n < res_w; n++)
      {
          const float t1 = t0 + t_step;
          const int v1_idx = (int)(t0 * item_count + 0.5f);
          IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
          const float v1 = data[(v1_idx + values_offset + 1) % values_count];
          const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale) );

          // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
          ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
          ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);
          //ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(tp0.x, histogram_zero_line_t));

          window->DrawList->AddLine(pos0, pos1, col_base);

          t0 = t1;
          tp0 = tp1;
      }*/
    /*int lastsamples=0;
      int hsamples=(int)(inner_bb.Max.x-inner_bb.Min.x);
      for (int n=1; n<hsamples; n++) {
          double fixme=(float)(pow(2048,(float)n/(float)hsamples)-1)/2047.0f;
          int samples=fixme*values_count;
          IM_ASSERT(samples >= 0 && samples < values_count);
          if(samples==lastsamples || samples < 0 || samples >= values_count) {
              continue;
          }
          float sum=0;
          for(int i=lastsamples; i++; i<samples) {
              sum+=data[(i + values_offset + 1) % values_count];
          }
          const float v1 = sum/(float)(samples-lastsamples);
          const float t1 = n/hsamples;
          const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale) );

          // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
          ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
          ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);
          //ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(tp0.x, histogram_zero_line_t));

          window->DrawList->AddLine(pos0, pos1, col_base);

          t0 = t1;
          tp0 = tp1;
        }*/
  }

  ImGui::PopClipRect();

  // Text overlay
  if (overlay_text)
    RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f,0.0f));

  if (label_size.x > 0.0f)
    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
}

template void ImGui::PlotSpectrum(const char* label, float* data, int nbins, int samplerate, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size);
template void ImGui::PlotSpectrum(const char* label, std::complex<float>* data, int nbins, int samplerate, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size);
