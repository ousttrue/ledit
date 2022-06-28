#pragma once
#include <stdint.h>

class VBO {
  uint32_t handle;

public:
  VBO();
  ~VBO();
  void bind();
  void unbind();
  void dynamicData(size_t size);
  void upload(const void *data, size_t size);
};

class VAO {
  uint32_t handle;

public:
  VAO();
  ~VAO();
  void bind();
  void unbind();
  void drawTriangleStrip(uint32_t count);
  void drawTriangleStripInstance(uint32_t count, uint32_t instance);
};
