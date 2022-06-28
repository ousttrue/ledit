#pragma once
#include <stdint.h>
#include <string>
#include <vector>

struct VertexLayout {
  uint32_t type; // GL_FLOAT
  int size;      // 1, 2, 3, 4(scalar, vec2, vec3, vec4)
  size_t offset;
  uint32_t divisor;
};

class Drawable {

  class DrawableImpl *_impl = nullptr;
  Drawable(const Drawable &) = delete;
  Drawable &operator=(const Drawable &) = delete;

public:
  Drawable(const std::vector<uint8_t> &vertex,
           const std::vector<uint8_t> &fragment,
           const std::vector<std::vector<uint8_t>> &others, size_t stride,
           const VertexLayout *layouts, size_t len, size_t dataSize);
  ~Drawable();

  void use();
  void set(const std::string &name, float x, float y);
  void set(const std::string &name, float x, float y, float z, float w);
  void drawTriangleStrip(int count);
  void drawUploadInstance(const void *data, size_t len, int count,
                          int instance);
};
