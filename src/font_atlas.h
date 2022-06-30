#pragma once
#include "renderchar.h"
#include <string>
#include <vector>
#include <stdint.h>

class FontAtlas {
  class FontAtlasImpl *_impl = nullptr;

public:
  FontAtlas(const std::string &path, uint32_t fontSize);
  ~FontAtlas();
  void readFont(const std::string &path, uint32_t fontSize);
  void renderFont(uint32_t fontSize);
  std::vector<float> *getAllAdvance(const std::u16string &line, int y);
  float getAdvance(uint16_t cp);
  float getAdvance(const std::string &line);
  float getAdvance(const std::u16string &line);
  float getHeight() const;
  class Texture *getTexture() const;
  RenderChar render(char16_t c, float x = 0.0, float y = 0.0,
                    Vec4f color = vec4fs(1));
};
