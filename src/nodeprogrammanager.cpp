#include "nodeprogrammanager.h"

#include "app.h"
#include "instreammanager.h"
#include "logs.h"
#include "nodegraph.h"
#include "profiler.h"
#include "ringbuffer.h"
#include "shadernodeprogram.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <sstream>

double euclideanModulo(double dividend, double divisor) {
    return fmod((fmod(dividend, divisor) + divisor), divisor);
}

void NodeProgramManager::loadTypes() {
  audioInType.emplace(library.addProgramType("Audio Input", {
      {"audio", tn_PortRoleOutput, tn_PortTypeSampleBlock}
},[](tn_State* state, unsigned long idx)
  {
      InStreamManager& ism=App::get().instreammanager;
      size_t bufsize=ism.blocksize*sizeof(float);
      void* payload=tn_getPM(state->portState[0], idx).raw;
      memcpy(payload, (void*)ism.inStreamBuffer.newest(), bufsize);
  }));
  // HACK
  audioInType->meta->type=NodeProgramMetadata::Type::Source;
  displayType.emplace(library.addProgramType("Display", {
      {"visuals", tn_PortRoleInput, tn_PortTypeShader}
    },nullptr));

  library.addProgramType(plotIdentifier, {
      {"scalar", tn_PortRoleInput, tn_PortTypeScalar}
    },nullptr);

  library.addProgramType(plotIdentifier, {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum}
    },nullptr);

  library.addProgramType("FFT", {
      {"waveform", tn_PortRoleInput, tn_PortTypeSampleBlock},
      {"spectrum", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_State* state, unsigned long idx){
    if(state->portState[0].portData.ring_buffer) { // FIXME: Ensure no such test is needed anymore
      void* payload0=tn_getPM(state->portState[0], idx).raw;
      void* payload1=tn_getPM(state->portState[1], idx).raw;

      float* inbuf=(float*)(payload0);
      fftwf_complex* outbuf=(fftwf_complex*)payload1;

      fftwf_plan& fftPlan=App::get().instreammanager.fftPlan;
      fftwf_execute_dft_r2c(
            fftPlan,
            inbuf,
            outbuf
            );
    }
  });

  // https://web.media.mit.edu/~tristan/phd/dissertation/chapter3.html
  // E. Terhardt. Calculating virtual pitch. Hearing Research, 1:155-182, 1979.
  library.addProgramType("Equal Loudness", {
      {"raw spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"eq spectrum", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_State* state, unsigned long idx){
    std::complex<float>* inbuf =(std::complex<float>*)tn_getPM(state->portState[0], idx).frequency_window.buffer;
    std::complex<float>* outbuf=(std::complex<float>*)tn_getPM(state->portState[1], idx).frequency_window.buffer;

    InStreamManager& ism=App::get().instreammanager;
    const int fftElem=ism.fftElem;

    for (int k = 0; k < fftElem; ++k) {
      double fkhz=(ism.sample_rate*0.001)*k/(2.*(fftElem-1));
      double att=-3.64 * pow(fkhz, -0.8) + 6.5 * exp(-0.6 * pow(fkhz - 3.3, 2)) - 0.001 * pow(fkhz, 4);
      outbuf[k]=inbuf[k] * (float)pow(10, att / 20);;
    }
  });

  library.addProgramType("A-Weight", {
      {"raw spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"eq spectrum", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_State* state, unsigned long idx){
    std::complex<float>* inbuf =(std::complex<float>*)tn_getPM(state->portState[0], idx).frequency_window.buffer;
    std::complex<float>* outbuf=(std::complex<float>*)tn_getPM(state->portState[1], idx).frequency_window.buffer;

    InStreamManager& ism=App::get().instreammanager;
    const int fftElem=ism.fftElem;

    for (int k = 0; k < fftElem; ++k) {
      const float fac1khz=0.7943411775965269; // factor at 1kHz for normalization
      const float p1=12194.217*12194.217;
      const float p2=20.598997*20.598997;
      const float p3=107.65265*107.65265;
      const float p4=737.86223*737.86223;
      float f=ism.sample_rate*k/(2.*(fftElem-1));
      float fac=(p1 * pow(f,4)) / ((f*f + p2) * sqrt((f*f + p3) * (f*f + p4)) * (f*f + p1));
      // fac/=fac1khz; << skip normalization to avoid clipping!
      outbuf[k]=inbuf[k] * fac;
    }
  });

  library.addProgramType("Harmonic-Percussive-Separation", {
      {"input", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"harmonic", tn_PortRoleOutput, tn_PortTypeSpectrum},
      {"percussive", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_State* state, unsigned long idx){
    if(state->portState[0].portData.ring_buffer) { // FIXME: Ensure no such test is needed anymore
      std::complex<float>* input     =(std::complex<float>*)tn_getPM(state->portState[0], idx).frequency_window.buffer;
      std::complex<float>* harmonic  =(std::complex<float>*)tn_getPM(state->portState[1], idx).frequency_window.buffer;
      std::complex<float>* percussive=(std::complex<float>*)tn_getPM(state->portState[2], idx).frequency_window.buffer;

      int fftElem=App::get().instreammanager.fftElem;

      { // Harmonic–Percussive Separation (HPS)
          const int lh=9; //=13;
          const int lp=29; //=9;
          for (int k = 0; k < fftElem; ++k) {
              // calculate vertical median y_p
              float y_p=0;
              {
                  int left = std::max(0, k-lp/2);
                  left = std::min(left, fftElem-k-1);
                  float tmp_arr[fftElem]; // sized to theoretic max because lp is variable
                  for (int i=0; i<lp; i++) {
                      tmp_arr[i]=std::abs(input[left+i]);
                  }
                  std::nth_element(tmp_arr, tmp_arr+lp/2, tmp_arr+lp);
                  y_p = tmp_arr[lp/2];
              }
              // calculate horizontal median y_h
              float y_h=0;
              {
                  float tmp_arr[lh];
                  for (int i=0; i<lh; i++) {
                    auto lookback=(std::complex<float>*)tn_getPM(state->portState[0], idx+i).frequency_window.buffer;
                    tmp_arr[i]=std::abs(lookback[k]);
                  }
                  std::nth_element(tmp_arr, tmp_arr+lh/2, tmp_arr+lh);
                  y_h = tmp_arr[lh/2];
              }

              const float eps=0.01;

              // calculate the vertical weighting function M_p
              float M_p=(y_p+eps/2)/(y_h+y_p+eps);

              // calculate the horizontal weighting function M_h
              float M_h=(y_h+eps/2)/(y_h+y_p+eps);

              harmonic[k]  =input[k]*M_h;
              percussive[k]=input[k]*M_p;
          }
        }
      }
  });

  // flux a.k.a. spectrum energy
  library.addProgramType("flux", {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"flux", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      std::complex<float>* current=(std::complex<float>*)tn_getPM(state->portState[0], idx  ).frequency_window.buffer;
      std::complex<float>* last   =(std::complex<float>*)tn_getPM(state->portState[0], idx+1).frequency_window.buffer;
      double* outbuf=tn_getPM(state->portState[1], idx).scalar.v;
      int fftElem=App::get().instreammanager.fftElem;
      double a=0;
      for (int k = 0; k < fftElem; ++k) {
        double d = std::abs(current[k])-std::abs(last[k]);
        a += (d+std::abs(d))/2.0;
      }
      *outbuf=a;
  });

  // flux a.k.a. spectrum energy
  library.addProgramType("RCD", {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"odf", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      std::complex<float>* current=(std::complex<float>*)tn_getPM(state->portState[0], idx  ).frequency_window.buffer;
      std::complex<float>* last   =(std::complex<float>*)tn_getPM(state->portState[0], idx+1).frequency_window.buffer;
      std::complex<float>* lastl   =(std::complex<float>*)tn_getPM(state->portState[0], idx+2).frequency_window.buffer;
      double* outbuf=tn_getPM(state->portState[1], idx).scalar.v;
      int fftElem=App::get().instreammanager.fftElem;
      double a=0;
      for (int k = 0; k < fftElem; ++k) {
        double d=0;
        if(std::abs(current[k])>=std::abs(last[k])) {
          auto lastphase=std::arg(last[k]);
          auto lastphasediff=lastphase-std::arg(lastl[k]);
          lastphasediff += (lastphasediff>M_PI) ? -2*M_PI : (lastphasediff<=-M_PI) ? 2*M_PI : 0;
          auto targetval=std::abs(last[k])*std::exp(lastphase+lastphasediff);
          d = std::abs(current[k]-targetval);
        }
        a += d;
      }
      *outbuf=a;
  });

  library.addProgramType("sum", {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"sum", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      std::complex<float>* inbuf=(std::complex<float>*)tn_getPM(state->portState[0], idx).frequency_window.buffer;
      double* outbuf=tn_getPM(state->portState[1], idx).scalar.v;
      int fftElem=App::get().instreammanager.fftElem;
      double a=0;
      for (int k = 0; k < fftElem; ++k) {
        a += std::abs(inbuf[k]);
      }
      *outbuf=a;
  });

  library.addProgramType("peak harmonic", {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"semitone", tn_PortRoleOutput, tn_PortTypeScalar},
      {"amplitude", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      std::complex<float>* inbuf=(std::complex<float>*)tn_getPM(state->portState[0], idx).frequency_window.buffer;
      double* semitone=tn_getPM(state->portState[1], idx).scalar.v;
      double* amplitude=tn_getPM(state->portState[2], idx).scalar.v;
      int fftElem=App::get().instreammanager.fftElem;
      double semitones[12]={0};
      InStreamManager& ism=App::get().instreammanager;
      int sample_rate=ism.sample_rate;
      for (int k = 1; k < fftElem; ++k) {
        double fhz=sample_rate*k/(2.*(fftElem-1));
        double n=12.0*log2(fhz/440);
        double o=euclideanModulo(n,12);
        int lower=(int)o;
        int higher=(lower+1)%12;
        double fracHigh=(double)o-lower;
        double a=std::abs(inbuf[k]);
        semitones[lower] +=(1.0-fracHigh)*a;
        semitones[higher]+=     fracHigh *a;
      }
      int smax=0;
      for (int i=1; i<12; i++) {
        if(semitones[i]>semitones[smax]) {
          smax=i;
        }
      }
      *semitone=smax;
      *amplitude=semitones[smax];
  });

  library.addProgramType("integral", {
      {"in", tn_PortRoleInput, tn_PortTypeScalar},
      {"out", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      double* inbuf  =tn_getPM(state->portState[0], idx).scalar.v;
      double* outbuf =tn_getPM(state->portState[1], idx).scalar.v;
      double* lastout=tn_getPM(state->portState[1], idx+1).scalar.v;

      *outbuf=*lastout + *inbuf;
  });

  library.addProgramType("square", {
      {"amplitude", tn_PortRoleInput, tn_PortTypeScalar},
      {"power", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_State* state, unsigned long idx){
      double* inbuf  =tn_getPM(state->portState[0], idx).scalar.v;
      double* outbuf =tn_getPM(state->portState[1], idx).scalar.v;

      *outbuf=*inbuf * *inbuf;
  });
}

bool NodeProgramManager::buildInvocationList() {
  Profiler profiler("bip");
  std::vector<Node*> inzero; // Audio and Time
  std::vector<Node*> outzero; // Displays (incl. textures)

  // removed as these can go in buildInvocationList->parametrizedFragments directly?:
  // std::vector<Node*> infragment; // fragment nodes with scalar input
  std::unordered_map<nodeid,int> deg;
  InvocationList* buildInvocationList=new InvocationList();
  for(auto& node:nodegraph.nodes) {
    auto type=node.second.getProgramType().meta->type;
    if(type==NodeProgramMetadata::Type::Source) {
      inzero.push_back(&node.second);
    } else if (type==NodeProgramMetadata::Type::Display) {
      outzero.push_back(&node.second);
      Logs::get().logf(logSeverity_Debug,"Outzero", "%s", node.second.getProgramDescriptor()->identifier);
    } else if(type==NodeProgramMetadata::Type::PixelShader || type==NodeProgramMetadata::Type::CoordShader) {
      buildInvocationList->parametrizedFragments.push_back(node.second.getInstance());
    }
  }
  while(!inzero.empty()) {
    Node* node=inzero.back();
    inzero.pop_back();
    auto type=node->getProgramType().meta->type;
    //if(type!=NodeProgramMetadata::Type::PixelShader && type!=NodeProgramMetadata::Type::CoordShader) {
    if(type==NodeProgramMetadata::Type::Runnable || type==NodeProgramMetadata::Type::Source) {
      buildInvocationList->list.push_back(node->getInstance());
      Logs::get().logf(logSeverity_Debug,"Nodegraph", "%i", node->id);
      for(NodeOutput* out: node->out) {
        for(auto& t: out->targets) {
          Node* child=t->target;
          if(!deg.count(child->id)) {
            int count=0;
            for(auto& input:child->in) {
              if(input->source) {
                count++;
              }
            }
            deg[child->id]=count-1;
          } else {
            deg[child->id]--;
          }
          if(deg[child->id]==0) {
            inzero.push_back(child);
            deg.erase(child->id);
          }
        }
      }
    }
  }
  for(auto fragmentNode:buildInvocationList->parametrizedFragments) {
    auto* fragmentNodeUserdata=(ShaderNodeProgram::Userdata*)fragmentNode.getProgramState()->userdata;
    if(fragmentNodeUserdata) {
      delete fragmentNodeUserdata;
    }
    fragmentNode.getProgramState()->userdata=(void**)new ShaderNodeProgram::Userdata(); // HACK
  }
  while(!outzero.empty()) {
    Node* node=outzero.back();
    outzero.pop_back();
    NodeOutput* source=node->in[0]->source;
    if(source) {
      std::stringstream preamble;
      std::stringstream body;
      Logs::get().logf(logSeverity_Debug, "Nodegraph", "Build a shader for Display %i", node->id);

      Node* shadernode=source->source;

      const char* shadername=shadernode->getProgramDescriptor()->identifier;
      Logs::get().logf(logSeverity_Debug, "Nodegraph", "Source is %s %i", shadername, source->source->id);
      auto& shadermanager=App::get().shadermanager;
      FragmentShader& frag=shadermanager.shaderfiles.at(shadername);
      Logs::get().logf(logSeverity_Debug, "Nodegraph", "Random metric of the shader is %i", frag.parameters.size());

      preamble<<"uniform vec2 resolution;\n\n";
      preamble<<frag.preambleString();


      body<<"void main() {\n"
         <<"vec2 uv = gl_FragCoord.xy/resolution.xy;\n";

      std::string shaderResultId="_"+std::to_string(shadernode->id)+"_result";
      body<<"vec3 "<<shaderResultId<<";\n";
      body<<frag.name<<"("<<shaderResultId<<", uv";

      for(int i=0; i<shadernode->in.size(); i++) {
        auto* input=shadernode->in[i];
        auto* pd=input->getPortDescriptor();
        if(pd->type==tn_PortTypeShader) {
          if(!input->source) {
            body<<", ";
            std::string identifier="_"+std::to_string(input->id)+"_placeholder";
            preamble<<"uniform vec3 "<<identifier<<";\n";
            body<<identifier;
          }
        }

        // Commented out: float inputs are uniforms
        /*else if(pd->type==tn_PortTypeScalar) {
          if(!input->source) {
            std::string identifier="_"+std::to_string(input->id)+"_placeholder";
            preamble<<"uniform float "<<identifier<<";\n";
            if(i!=0) {
              body<<", ";
            }
            body<<identifier;
          }
        }*/

      }
      body<<");\n";
      body<<"gl_FragColor=vec4("<<shaderResultId<<",1.0);\n";
      body<<"}\n";

      std::string src=preamble.str()+body.str();
      Logs::get().logf(logSeverity_Debug, "Nodegraph", "main.frag might be like\n%s", src.c_str());

      GLuint shader_object_id = glCreateShader(GL_FRAGMENT_SHADER);
      GLint compile_status;

      if(shader_object_id == 0) {
        Logs::get().log(logSeverity_Err, "Nodegraph", "Could not create Shader");
      }
      const GLchar * csource=src.c_str();
      glShaderSource(shader_object_id, 1, &csource, NULL);

      glCompileShader(shader_object_id);

      glGetShaderiv(shader_object_id, GL_COMPILE_STATUS, &compile_status);
      GLchar log_buffer[1024];
      glGetShaderInfoLog(shader_object_id, 1024, NULL, log_buffer);
      Logs::get().logf(logSeverity_Debug, "GL", "Compiled shader with status [%i]: %s", compile_status, log_buffer);
      GLuint program=glCreateProgram();
      glAttachShader(program, shadermanager.vertex_shader);
      glAttachShader(program, shader_object_id);
      glAttachShader(program, frag.shader);
      glLinkProgram(program);
      GLint linkStatus;
      glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

      if (linkStatus != GL_TRUE) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0) {
          GLchar* infoLog = new GLchar[logLength];
          glGetProgramInfoLog(program, logLength, NULL, infoLog);
          Logs::get().logf(logSeverity_Err, "Nodegraph", "Linkage failed: %s", infoLog);
          delete[] infoLog;
        } else {
          Logs::get().log(logSeverity_Err, "Nodegraph", "Linkage failed, no log available");
        }
      }

      // FIXME: Resource leak
      // FIXME: Memory leaks below this line
      //glDeleteShader(shader_object_id);
      //glDeleteProgram(program);

      DisplayPayload* dp;
      if(node->getInstance().getProgramState()->userdata) {
        dp=(DisplayPayload*)node->getInstance().getProgramState()->userdata;
      } else {
        dp=new DisplayPayload();
        node->getInstance().getProgramState()->userdata=(void**)dp;
      }
      dp->inv.program=program;
      dp->inv.shader=shader_object_id;
      dp->inv.resolution_location=glGetUniformLocation(program, "resolution");
      dp->inv.vpos_location=glGetAttribLocation(program, "vPos");
      // FIXME: Memory leaks above this line


      ((ShaderNodeProgram::Userdata*)shadernode->getInstance().getProgramState()->userdata)->targetDisplays.push_back(dp);

      buildInvocationList->displays.push_back(node->getInstance());
    }
  }
  if (!deg.empty()) {
    Logs::get().log(logSeverity_Debug,"Nodegraph","avoided cyclic graph");
    return false;
  } else {
    //std::string output;
    //for (Node* node : result) {
    //    output += std::to_string(node->id) + " ";
    //}
    //Logs::get().logf(logSeverity_Debug,"Nodegraph","%s", output.c_str());

    for(NodeProgramInstanceWrapper& i:buildInvocationList->parametrizedFragments) {
      i.linkBuffers();
    }
    for(NodeProgramInstanceWrapper& i:buildInvocationList->list) {
      i.linkBuffers();
    }

    newInvocationList=std::move(buildInvocationList);
    return true;
  }
}
