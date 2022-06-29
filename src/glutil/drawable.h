#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

struct VertexLayout {
  // uint32_t type; // GL_FLOAT
  int size;      // 1, 2, 3, 4(scalar, vec2, vec3, vec4)
  size_t offset;
  uint32_t divisor;
};

class Drawable {

  class DrawableImpl *_impl = nullptr;
  Drawable(const Drawable &) = delete;
  Drawable &operator=(const Drawable &) = delete;


public:
  Drawable(const std::shared_ptr<class Shader> &shader, size_t stride,
           const VertexLayout *layouts, size_t len, size_t dataSize);
  ~Drawable();

  void use();
  void set(const std::string &name, float x, float y);
  void set(const std::string &name, float x, float y, float z, float w);
  void drawTriangleStrip(int count);
  void drawUploadInstance(const void *data, size_t len, int count,
                          int instance);
};
