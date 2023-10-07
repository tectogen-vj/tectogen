#pragma once

#include <GL/glew.h> //GLUint
#include <cstring>
#include <filesystem>
#include <glm/glm.hpp> //vec2
#include <string>
#include <vector>

class Shader {
private:
  struct M {
    GLuint shader;
    M();
    ~M();
  };
  std::shared_ptr<M> m;
  const GLenum type;
public:
  const std::string source;
  static const glm::vec2 screenPlaneVertices[4];
  static const char* vertex_shader_text;
  void compile_shader();
  inline operator GLuint() const { return m->shader; }
  Shader(const GLenum type, const std::string source);
};

class FragmentShader {
public:
  struct Param {
    enum class Storage { // "Storage Qualifier"
      none,
      in,
      out,
      inout
    };
    enum class Type { // "Type Specifier"
      none,
      vec2,
      vec3
    };
    Storage s;
    Type t;
    std::string v; // "Variable Identifier"
    Param(Storage s, Type t, const std::string v):s(s),t(t),v(v) {}
    Param():s(Param::Storage::none),t(Param::Type::none){}
    std::string storageString() const {
      switch(s) {
        case Storage::in: return "in"; break;
        case Storage::out: return "out"; break;
        case Storage::inout: return "inout"; break;
        default: throw;
      }
    }
    std::string typeString() const {
      switch(t) {
        case Type::vec2: return "vec2"; break;
        case Type::vec3: return "vec3"; break;
        default: throw;
      }
    }
    std::string toString() const;
  };

  Shader shader;
  const std::string filename;
  const std::string name;
  /*inline void compile_shader() {
    shader.compile_shader();
  }*/
  FragmentShader(const std::filesystem::directory_entry file);
  bool parsed=false;
  bool parse();
  std::vector<std::string> uniforms;
  std::vector<Param> parameters;
  bool isCoordShader();
  bool isPixelShader();
  std::string signatureString() const;
  std::string uniformsString() const;
  std::string preambleString() const;
};

