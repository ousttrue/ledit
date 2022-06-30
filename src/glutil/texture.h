#pragma once
#include <memory>
#include <stdint.h>

class Texture {
  class TextureImpl *_impl = nullptr;
  Texture(int width, int height);

public:
  ~Texture();
  static std::shared_ptr<Texture> create(int w, int h);
  void bind(uint32_t slot);
  void unbind();
  uint32_t getHandle() const;
  void subImage(int xOffset, const void *p, int width, int height);
};
