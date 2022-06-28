#include "font_atlas.h"

FontAtlas::FontAtlas(const std::string &path, uint32_t fontSize) {
  if (FT_Init_FreeType(&ft)) {
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
              << std::endl;
    return;
  }
  readFont(path, fontSize);
}
