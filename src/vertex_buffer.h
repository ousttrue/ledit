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
