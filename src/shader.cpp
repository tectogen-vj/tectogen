#include "shader.h"

#include "logs.h"

#include <GL/glew.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>

const glm::vec2 Shader::screenPlaneVertices[4] =
{
  { -1.f, -1.f},
  { -1.f,  1.f},
  {  1.f, -1.f},
  {  1.f,  1.f}
};
const char* Shader::vertex_shader_text =
    "attribute vec2 vPos;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(vPos, 0.0, 1.0);\n"
    "}\n";

void Shader::compile_shader() {
  //assert(source != NULL);
  GLuint shader_object_id = glCreateShader(type);
  GLint compile_status;

  if(shader_object_id == 0) {
    Logs::get().log(logSeverity_Err, "GL", "Could not create Shader");

  }

  const GLchar * csource=source.c_str();
  glShaderSource(shader_object_id, 1, &csource, NULL);

  glCompileShader(shader_object_id);

  glGetShaderiv(shader_object_id, GL_COMPILE_STATUS, &compile_status);



  //GLint log_length;
  //glGetProgramiv(shader_object_id, GL_INFO_LOG_LENGTH, &log_length);
  GLchar log_buffer[1024];
  glGetShaderInfoLog(shader_object_id, 1024, NULL, log_buffer);

  // compile_status TRUE -> successful compilation
  Logs::get().logf(logSeverity_Debug, "GL", "Compiled shader with status [%i]: %s", compile_status, log_buffer);

  //assert(compile_status != 0);

  m->shader=shader_object_id;
}

Shader::Shader(const GLenum type, const std::string source):type(type), source(source), m(new M()) {
}

Shader::M::M() : shader(0) {}

Shader::M::~M() {
  if(shader) {
    glDeleteShader(shader);

  }
  Logs::get().log(logSeverity_Debug, "Shader", "Shader deleted");
}


std::string readFile(const std::string& filename) {

  std::ifstream s(filename);
  std::string content((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
  s.close();
  return content;
}

FragmentShader::FragmentShader(const std::filesystem::directory_entry file)
  : shader(GL_FRAGMENT_SHADER, readFile(file.path())),
    filename(file.path().filename().string()),
    name(filename.substr(0, filename.find('.')))
{

}


struct Parser {
  size_t pos=0;
  const char* s;
  Parser(const char* s):s(s){}
  bool whitespace() {
    if(s[pos]=='\0' || !std::isspace(s[pos])) {
      return false;
    }
    do {
      pos++;
    } while(s[pos]!='\0' && std::isspace(s[pos]));
    return true;
  }
  bool cont(const std::string w) {
    if(strncmp(s+pos, w.c_str(), w.length())==0) {
      pos+=w.length();
      return true;
    }
    return false;
  }
  bool cont(const char c) {
    if(s[pos]==c) {
      pos++;
      return true;
    }
    return false;
  }
  std::optional<std::string> identifier() {
    const char c=s[pos];
    if(
       c=='_' ||
       c>='a' && c<='z' ||
       c>='A' && c<='Z'
       ) {
      const size_t start=pos;
      pos++;
      while(
            s[pos]=='_' ||
            s[pos]>='a' && s[pos]<='z' ||
            s[pos]>='A' && s[pos]<='Z' ||
            s[pos]>='0' && s[pos]<='9'
            ) {
        pos++;
      } // identifier complete
      return std::string(s+start,pos-start);
    }
    return std::nullopt;
  }
  bool end() {
    return s[pos]=='\0';
  }
  void skipline() {
    while(s[pos]!='\0') {
      if(s[pos]=='\\' && s[pos+1]!='\0') {
         pos++;
      } else if(s[pos]=='\n') {
        pos++;
        return;
      }
      pos++;
    }
  }
};

bool FragmentShader::parse() {
  // we're looking for a function named like the shader file but excluding the .frag (or anything before the first .)

  size_t pos=0; // read through the shader source for "markers"

  Parser p(shader.source.c_str());
  while(!p.end()) {
    p.whitespace();
    if(p.cont("void") && parameters.empty()) {
      if(p.whitespace()) {
        if(p.cont(name)) {
          p.whitespace();
          if(p.cont('(')) {
            do {
              p.whitespace();
              Param param;
              if(p.cont("inout")) {
                param.s=Param::Storage::inout;
              } else if(p.cont("out")) {
                param.s=Param::Storage::out;
              } else if(p.cont("in")) {
                param.s=Param::Storage::in;
              }
              if(param.s!=Param::Storage::none && p.whitespace()) {
                if(p.cont("vec2")) {
                  param.t=Param::Type::vec2;
                } else if(p.cont("vec3")) {
                  param.t=Param::Type::vec3;
                }
                if(param.t!=Param::Type::none && p.whitespace()) {
                  if(auto id=p.identifier()) {
                    param.v=id.value();
                    parameters.push_back(param);
                  }
                }
              }
              p.whitespace();
            } while(p.cont(','));
            parsed=true;
            return true;
          }
        }
      }
    } else if(p.cont("uniform")) {
      if(p.whitespace()) {
        if(p.cont("float")) {
          if(p.whitespace()) {
            if(auto id=p.identifier()) {
              p.whitespace();
              if(p.cont(';')) {
                uniforms.push_back(id.value());
              }
            }
          }
        }
      }
    } else {
      p.skipline();
    }
  }
  return false;
}

bool FragmentShader::isCoordShader() {
  // Must have exactly two parameter
  if(parameters.size()!=2) {
    return false;
  }
  // Must have one input and one output parameter
  if(
     parameters[0].s==Param::Storage::in && parameters[1].s!=Param::Storage::out ||
     parameters[0].s==Param::Storage::out && parameters[1].s!=Param::Storage::in
  ) {
    return false;
  }
  // All parameters must be of type vec2
  if(parameters[0].t!=Param::Type::vec2 || parameters[1].t!=Param::Type::vec2) {
    return false;
  }
  return true;
}

bool FragmentShader::isPixelShader() {
  bool const minTwoParam=parameters.size()>=2;
  bool const oneOutVec3=std::count_if(parameters.begin(), parameters.end(), [](Param& p) {
      return p.s==Param::Storage::out && p.t==Param::Type::vec3;
    })==1;
  bool const oneOut=std::count_if(parameters.begin(), parameters.end(), [](Param& p) {
      return p.s==Param::Storage::out;
    })==1;
  bool const maxOneVec2In=std::count_if(parameters.begin(), parameters.end(), [](Param& p) {
      return p.s==Param::Storage::in && p.t==Param::Type::vec2;
    })<=1;
  return minTwoParam && oneOutVec3 && oneOut && maxOneVec2In;
}

std::string FragmentShader::signatureString() const
{
  std::stringstream stream;
  stream<<"void "<<name<<"(";

  for (size_t i = 0; i < parameters.size(); ++i) {
      if (i > 0) {
          stream << ", ";
      }
      stream << parameters[i].toString();
  }
  stream<<");";
  return stream.str();
}

std::string FragmentShader::uniformsString() const
{
  std::stringstream stream;
  for(auto& el:uniforms) {
    stream<<"uniform float "<<el<<";\n";
  }
  return stream.str();
}

std::string FragmentShader::preambleString() const
{
  return uniformsString()+"\n"+signatureString()+"\n";
}

std::string FragmentShader::Param::toString() const
{
  return storageString()+' '+typeString()+' '+v;
}
