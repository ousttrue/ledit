#pragma once
#include "la.h"
#include "renderchar.h"
#include <memory>
#include <vector>

class Renderer {

  std::vector<RenderChar> entries;

public:
  std::vector<RenderChar> &render(int width, int height,
                                  const std::shared_ptr<class Document> &cursor,
                                  const std::shared_ptr<class FontAtlas> &atlas,
                                  int fontWidth, const Vec4f &color);
};
