#include "nodeprogrammanager.h"

#include "app.h"
#include "instreammanager.h"
#include "logs.h"
#include "nodegraph.h"
#include "profiler.h"
#include "ringbuffer.h"
#include "shadernodeprogram.h"

#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <sstream>

void NodeProgramManager::loadTypes() {
  audioInType.emplace(library.addProgramType("Audio Input", {
      {"audio", tn_PortRoleOutput, tn_PortTypeSampleBlock}
                                             },[](tn_Userdata handle, tn_State* state){
      InStreamManager& ism=App::get().instreammanager;
      size_t bufsize=ism.blocksize*sizeof(float);
                        void* payload=*state->portState[0].payload_symbol_to_be_obsoleted;
      memcpy(payload, (void*)ism.inStreamBuffer.newest(), bufsize);
  }));
  // HACK
  audioInType->meta->type=NodeProgramMetadata::Type::Source;
  displayType.emplace(library.addProgramType("Display", {
      {"visuals", tn_PortRoleInput, tn_PortTypeShader}
    },nullptr));

  library.addProgramType("FFT", {
      {"waveform", tn_PortRoleInput, tn_PortTypeSampleBlock},
      {"spectrum", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_Userdata handle, tn_State* state){
    if(state->portState[0].payload_symbol_to_be_obsoleted) {
      void* payload0=*(state->portState[0].payload_symbol_to_be_obsoleted);
        float* inbuf=(float*)(payload0);
        void* payload1=*(state->portState[1].payload_symbol_to_be_obsoleted);
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
  // E. Terhardt. Calculating virtual pitch. Hearing Research, 1:155–182, 1979.
  library.addProgramType("Equal Loudness", {
      {"raw spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"eq spectrum", tn_PortRoleOutput, tn_PortTypeSpectrum}
                         },[](tn_Userdata handle, tn_State* state){
    if(state->portState[0].payload_symbol_to_be_obsoleted) {
      void* payload0=*(state->portState[0].payload_symbol_to_be_obsoleted);
        std::complex<float>* inbuf=(std::complex<float>*)(payload0);
        void* payload1=*(state->portState[1].payload_symbol_to_be_obsoleted);
        std::complex<float>* outbuf=(std::complex<float>*)payload1;

        InStreamManager& ism=App::get().instreammanager;
        const int fftElem=ism.fftElem;

        for (int k = 0; k < fftElem; ++k) {
          double fkhz=(ism.sample_rate*0.001)*k/(2.*(fftElem-1));
          double att=-3.64 * pow(fkhz, -0.8) + 6.5 * exp(-0.6 * pow(fkhz - 3.3, 2)) - 0.001 * pow(fkhz, 4);
          outbuf[k]=inbuf[k] * (float)pow(10, att / 20);;
        }
      }
  });

  library.addProgramType("sum", {
      {"spectrum", tn_PortRoleInput, tn_PortTypeSpectrum},
      {"sum", tn_PortRoleOutput, tn_PortTypeScalar}
                         },[](tn_Userdata handle, tn_State* state){
    if(state->portState[0].payload_symbol_to_be_obsoleted) {
      void* payload0=*(state->portState[0].payload_symbol_to_be_obsoleted);
        std::complex<float>* inbuf=(std::complex<float>*)(payload0);
        void* payload1=*(state->portState[1].payload_symbol_to_be_obsoleted);
        double* outbuf=(double*)payload1;
        int fftElem=App::get().instreammanager.fftElem;
        double a=0;
        for (int k = 0; k < fftElem; ++k) {
          a += std::abs(inbuf[k]);
        }
        *outbuf=a;
      }
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
    if(type!=NodeProgramMetadata::Type::PixelShader && type!=NodeProgramMetadata::Type::CoordShader) {
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
      for(std::optional<MultiBuffer>& j:*i.getBuffers()) {
        if(j.has_value()) {
          buildInvocationList->buffers.push_back(&j.value());
        }
      }
    }

    newInvocationList=std::move(buildInvocationList);
    return true;
  }
}
