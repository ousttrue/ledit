#include <glad.h>
#include <iostream>
#include <memory>
#include "shader.h"

static void checkCompileErrors(GLuint shader, std::string type) {
  GLint success;
  GLchar infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      std::cout
          << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::cout
          << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  }
}

static std::string shaderTypeString(GLuint shader) {
  switch (shader) {
  case GL_VERTEX_SHADER:
    return "GL_VERTEX_SHADER";
  case GL_FRAGMENT_SHADER:
    return "GL_FRAGMENT_SHADER";
  default:
    return "(Unknown)";
  }
}

static GLuint compileSimple(GLuint type, const std::vector<uint8_t> &content) {
  auto id = glCreateShader(type);
  const char *contentp = (const char *)content.data();
  glShaderSource(id, 1, &contentp, nullptr);
  glCompileShader(id);
  checkCompileErrors(id, shaderTypeString(type));
  return id;
}

struct ShaderImpl {
  GLuint handle;
  ShaderImpl() { handle = glCreateProgram(); }
  ~ShaderImpl() { glDeleteProgram(handle); }
};

///
/// Shader
///
Shader::Shader() : _impl(new ShaderImpl) {}
Shader::~Shader() { delete _impl; }

std::shared_ptr<Shader>
Shader::create(const std::vector<uint8_t> &vertex,
               const std::vector<uint8_t> &fragment,
               const std::vector<std::vector<uint8_t>> &others) {

  std::vector<uint32_t> shader_ids;

  auto vertex_shader = compileSimple(GL_VERTEX_SHADER, vertex);
  shader_ids.push_back(vertex_shader);
  auto fragment_shader = compileSimple(GL_FRAGMENT_SHADER, fragment);
  shader_ids.push_back(fragment_shader);

  auto shader = std::shared_ptr<Shader>(new Shader);

  glAttachShader(shader->_impl->handle, vertex_shader);
  glAttachShader(shader->_impl->handle, fragment_shader);
  for (auto &other : others) {
    auto shader_id = compileSimple(GL_VERTEX_SHADER, other);
    glAttachShader(shader->_impl->handle, shader_id);
    shader_ids.push_back(shader_id);
  }

  glLinkProgram(shader->_impl->handle);
  checkCompileErrors(shader->_impl->handle, "PROGRAM");

  for(auto shader: shader_ids)
  {
    glDeleteShader(shader);
  }
  return shader;
}

void Shader::set1f(const std::string &name, float v) {
  glUniform1f(glGetUniformLocation(_impl->handle, name.c_str()), v);
}
void Shader::set2f(const std::string &name, float x, float y) {
  glUniform2f(glGetUniformLocation(_impl->handle, name.c_str()), x, y);
}
void Shader::set4f(const std::string &name, float x, float y, float z,
                   float w) {
  glUniform4f(glGetUniformLocation(_impl->handle, name.c_str()), x, y, z, w);
}
void Shader::use() { glUseProgram(_impl->handle); }
