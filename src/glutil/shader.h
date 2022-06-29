#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class Shader {
  class ShaderImpl *_impl = nullptr;
  Shader();
  static std::shared_ptr<Shader>
  create(const std::vector<uint8_t> &vertex,
         const std::vector<uint8_t> &fragment,
         const std::vector<std::vector<uint8_t>> &others);

public:
  ~Shader();
  static std::shared_ptr<Shader> createText();
  static std::shared_ptr<Shader> createSelection();
  static std::shared_ptr<Shader> createCursor();

  void set1f(const std::string &name, float v);
  void set2f(const std::string &name, float x, float y);
  void set4f(const std::string &name, float x, float y, float z, float w);
  void use();
};
