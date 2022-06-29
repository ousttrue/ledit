#pragma once
#include <memory>

class Texture {
  class TextureImpl *_impl = nullptr;
  Texture(int width, int height);

public:
  ~Texture();
  static std::shared_ptr<Texture> create(int w, int h);
  uint32_t getHandle() const;
  void subImage(int xOffset, const void *p, int width, int height);
};
