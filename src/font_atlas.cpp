#include "font_atlas.h"
#include "la.h"
#include "glutil/shader.h"
#include "glutil/texture.h"
#include "base64.h"
#include "utils.h"
#include <ft2build.h>
#include <memory>
#include <stdint.h>
#include <utility>
#include FT_FREETYPE_H
#include <vector>
#include <map>
#include <iostream>
#include <assert.h>

struct CharacterEntry {
  float width = 0;
  float height = 0;
  float top = 0;
  float left = 0;
  float advance = 0;
  float offset = 0;
  std::vector<uint8_t> bitmap;
  int xPos = 0;
  char16_t c = 0;

  void load(char16_t c, FT_GlyphSlot glyph, int xOffset) {
    auto &bm = glyph->bitmap;
    this->width = bm.width;
    this->height = bm.rows;
    this->top = glyph->bitmap_top;
    this->left = glyph->bitmap_left;
    this->advance = glyph->advance.x >> 6;
    this->xPos = xOffset;
    this->c = c;
    this->bitmap.resize((int)width * (int)height);
    memcpy(bitmap.data(), glyph->bitmap.buffer, bitmap.size());

    // CharacterEntry entry;
    // auto bm = glyph->bitmap;
    // entry.width = bm.width;
    // entry.height = bm.rows;
    // entry.top = glyph->bitmap_top;
    // entry.left = glyph->bitmap_left;
    // entry.advance = glyph->advance.x >> 6;
    // entry.xPos = xOffset;
  }
};

struct FreeType {
  FT_Library ft;

  FreeType() {
    if (FT_Init_FreeType(&ft)) {
      assert(false);
    }
  }
  ~FreeType() { FT_Done_FreeType(ft); }
};

class FreeTypeFace {
  FT_Face _face;
  uint32_t fs = 0;
  FreeTypeFace(FT_Face face) : _face(face) {}

public:
  ~FreeTypeFace() { FT_Done_Face(_face); }
  static std::unique_ptr<FreeTypeFace> read(FT_Library ft,
                                            const std::string &path) {
    FT_Face face;
    int x = FT_New_Face(ft, path.c_str(), 0, &face);
    if (x) {
      std::cout << "ERROR::FREETYPE: Failed to load font " << x << std::endl;
      return {};
    }
    return std::unique_ptr<FreeTypeFace>(new FreeTypeFace(face));
  }

  void setSize(uint32_t height) {
    FT_Set_Pixel_Sizes(_face, 0, height);
    fs = height;
  }

  FT_GlyphSlot load(int i) {
    if (FT_Load_Char(_face, i, FT_LOAD_RENDER)) {
      std::cout << "Failed to load char: " << (char)i << "\n";
      return nullptr;
    }
    return _face->glyph;
  }
};

struct FontAtlasImpl {
  std::unique_ptr<FreeType> _ft;
  std::unique_ptr<FreeTypeFace> _face;
  FT_UInt atlas_width, atlas_height, smallest_top;
  std::shared_ptr<Texture> texture;
  std::map<char16_t, CharacterEntry> entries;
  std::map<int, std::vector<float>> linesCache;
  int xOffset = 0;
  std::map<int, std::u16string> contentCache;

  FontAtlasImpl() : _ft(new FreeType()) {}

  CharacterEntry *createEntry(uint16_t c, FT_GlyphSlot glyph) {
    // auto found = entries.find(c);
    // if (found != entries.end()) {
    //   return &found->second;
    // }

    auto kv_success = entries.insert(std::make_pair(c, CharacterEntry()));
    assert(kv_success.second);
    auto &entry = kv_success.first->second;
    entry.load(c, glyph, xOffset);

    entry.offset = (float)xOffset / (float)atlas_width;
    if (smallest_top == 0 && entry.top > 0)
      smallest_top = entry.top;
    else
      smallest_top =
          entry.top < smallest_top && entry.top != 0 ? entry.top : smallest_top;

    return &entry;
  }

public:
  void readFont(const std::string &path) {
    _face = FreeTypeFace::read(_ft->ft, path);
  }

  void renderFont(uint32_t fontSize) {
    entries.clear();
    linesCache.clear();
    atlas_width = 0;
    atlas_height = 0;
    smallest_top = 1e9;
    xOffset = 0;
    _face->setSize(fontSize);

    // TODO should this be here?
    for (int i = 0; i < 128; i++) {
      auto glyph = _face->load(i);
      if (glyph) {
        auto &bm = glyph->bitmap;
        atlas_width += bm.width;
        atlas_height = bm.rows > atlas_height ? bm.rows : atlas_height;
      }
    }
    atlas_width *= 2;

    // texture
    texture = Texture::create(atlas_width, atlas_height);

    for (uint16_t c = 0; c < 128; c++) {
      auto glyph = _face->load(c);
      if (!glyph) {
        std::cout << "Failed to load char: " << c << "\n";
        return;
      }
      auto entry = createEntry(c, glyph);
      texture->subImage(xOffset, glyph->bitmap.buffer, entry->width,
                        entry->height);
      xOffset += entry->width;
    }
  }

private:
  void lazyLoad(char16_t c) {
    if (entries.count(c)) {
      // already exists
      return;
    }

    auto glyph = _face->load(c);
    if (!glyph) {
      std::cout << "Failed to load char: " << (char)c << "\n";
      return;
    }

    auto entry = createEntry(c, glyph);

    auto old_width = atlas_width;
    auto old_height = atlas_height;
    atlas_width += glyph->bitmap.width;
    atlas_height =
        glyph->bitmap.rows > atlas_height ? glyph->bitmap.rows : atlas_height;
    entry->offset = (float)xOffset / (float)atlas_width;

    texture = Texture::create(atlas_width, atlas_height);

    // copy
    for (std::map<char16_t, CharacterEntry>::iterator it = entries.begin();
         it != entries.end(); ++it) {
      it->second.offset = (float)it->second.xPos / (float)atlas_width;
      texture->subImage(it->second.xPos, it->second.bitmap.data(),
                        it->second.width, it->second.height);
    }

    // add
    texture->subImage(xOffset, glyph->bitmap.buffer, entry->width,
                      entry->height);

    xOffset += entry->width;
  }

public:
  RenderChar render(char16_t c, float x = 0.0, float y = 0.0,
                    Vec4f color = vec4fs(1)) {
    if (c >= 128) {
      lazyLoad(c);
    }

    auto *entry = &entries[c];
    RenderChar r;
    float x2 = x + entry->left;
    float y2 = y - entry->top + (atlas_height);
    r.pos = vec2f(x2, -y2);
    r.size = vec2f(entry->width, -entry->height);
    r.uv_pos = vec2f(entry->offset, 0.0f);
    r.uv_size =
        vec2f(entry->width / (float)atlas_width, entry->height / atlas_height);
    r.fg_color = color;
    return r;
  }

  float getAdvance(char16_t c) { return entries[c].advance; }

  float getAdvance(const std::u16string &line) {
    float v = 0;
    std::u16string::const_iterator c;
    for (c = line.begin(); c != line.end(); c++) {
      if (*c >= 128)
        lazyLoad(*c);
      v += entries[*c].advance;
    }
    return v;
  }

  float getAdvance(const std::string &line) {
    float v = 0;
    std::string::const_iterator c;
    for (c = line.begin(); c != line.end(); c++) {
      char16_t cc = (char16_t)(*c);
      v += entries[cc].advance;
    }
    return v;
  }

  std::vector<float> *getAllAdvance(std::u16string line, int y) {
    if (linesCache.count(y)) {
      if (contentCache[y] == line) {
        return &linesCache[y];
      }
    }
    std::vector<float> values;
    std::u16string::const_iterator c;
    for (c = line.begin(); c != line.end(); c++) {
      if (*c >= 128)
        lazyLoad(*c);

      values.push_back(entries[*c].advance);
    }
    linesCache[y] = values;
    contentCache[y] = line;
    return &linesCache[y];
  }
};

///
/// FontAtlas
///
FontAtlas::FontAtlas(const std::string &path, uint32_t fontSize)
    : _impl(new FontAtlasImpl) {
  readFont(path, fontSize);
}
FontAtlas::~FontAtlas() { delete _impl; }
void FontAtlas::readFont(const std::string &path, uint32_t fontSize) {
  _impl->readFont(path);
  _impl->renderFont(fontSize);
}
void FontAtlas::renderFont(uint32_t fontSize) { _impl->renderFont(fontSize); }

float FontAtlas::getAdvance(uint16_t ch) { return _impl->getAdvance(ch); }
float FontAtlas::getAdvance(const std::string &line) {
  return _impl->getAdvance(line);
}
float FontAtlas::getAdvance(const std::u16string &line) {
  return _impl->getAdvance(line);
}
std::vector<float> *FontAtlas::getAllAdvance(const std::u16string &line,
                                             int y) {
  return _impl->getAllAdvance(line, y);
}
float FontAtlas::getHeight() const { return _impl->atlas_height; }
uint32_t FontAtlas::getTexture() const { return _impl->texture->getHandle(); }
RenderChar FontAtlas::render(char16_t c, float x, float y, Vec4f color) {
  return _impl->render(c, x, y, color);
}
