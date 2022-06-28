#pragma once
#include "../la.h"
#include "../utils.h"
#include <glad.h>
#include <stdint.h>
#include <iostream>
#include <vector>

struct SelectionEntry {
  Vec2f pos;
  Vec2f size;
};

class Shader {
public:
  GLuint pid;
  std::vector<GLuint> shader_ids;
  Shader(const std::vector<uint8_t> &vertex,
         const std::vector<uint8_t> &fragment,
         const std::vector<std::vector<uint8_t>> &others) {
    auto vertex_shader = compileSimple(GL_VERTEX_SHADER, vertex);
    auto fragment_shader = compileSimple(GL_FRAGMENT_SHADER, fragment);
    pid = glCreateProgram();
    glAttachShader(pid, vertex_shader);
    glAttachShader(pid, fragment_shader);
    shader_ids.push_back(vertex_shader);
    shader_ids.push_back(fragment_shader);
    for (auto &other : others) {
      auto shader_id = compileSimple(GL_VERTEX_SHADER, other);
      glAttachShader(pid, shader_id);
      shader_ids.push_back(shader_id);
    }
    glLinkProgram(pid);
    checkCompileErrors(pid, "PROGRAM");
  }
  void set2f(const std::string &name, float x, float y) {
    glUniform2f(glGetUniformLocation(pid, name.c_str()), x, y);
  }
  void set4f(const std::string &name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(pid, name.c_str()), x, y, z, w);
  }
  void set1f(const std::string &name, float v) {
    glUniform1f(glGetUniformLocation(pid, name.c_str()), v);
  }

  void use() { glUseProgram(pid); }

private:
  GLuint compileSimple(GLuint type, const std::vector<uint8_t> &content) {
    auto id = glCreateShader(type);
    const char *contentp = (const char *)content.data();
    glShaderSource(id, 1, &contentp, nullptr);
    glCompileShader(id);
    checkCompileErrors(id, shaderTypeString(type));
    return id;
  }

  void checkCompileErrors(GLuint shader, std::string type) {
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
  const std::string shaderTypeString(GLuint shader) {
    switch (shader) {
    case GL_VERTEX_SHADER:
      return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
      return "GL_FRAGMENT_SHADER";
    default:
      return "(Unknown)";
    }
  }
};
