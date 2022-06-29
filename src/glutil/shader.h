#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class Shader {
  class ShaderImpl *_impl = nullptr;
  Shader();

public:
  ~Shader();
  static std::shared_ptr<Shader>
  create(const std::vector<uint8_t> &vertex,
         const std::vector<uint8_t> &fragment,
         const std::vector<std::vector<uint8_t>> &others);
  void set1f(const std::string &name, float v);
  void set2f(const std::string &name, float x, float y);
  void set4f(const std::string &name, float x, float y, float z, float w);
  void use();
};
