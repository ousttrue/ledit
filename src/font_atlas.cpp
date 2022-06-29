#include "font_atlas.h"
#include "la.h"
#include "glutil/shader.h"
#include "glutil/texture.h"
#include "base64.h"
#include "utils.h"
#include "../third-party/freetype2/include/ft2build.h"
#include FT_FREETYPE_H
#include <vector>
#include <map>
#include <iostream>

struct CharacterEntry {
  float width;
  float height;
  float top;
  float left;
  float advance;
  float offset;
  uint8_t *data = nullptr;
  int xPos;
  char16_t c;
};

struct FontAtlasImpl {
  FT_UInt atlas_width, atlas_height, smallest_top;
  FT_Library ft;
  FT_Face face;
  bool wasGenerated = false;
  std::shared_ptr<Texture> texture;
  std::map<char16_t, CharacterEntry> entries;
  std::map<int, std::vector<float>> linesCache;
  uint32_t fs;
  int xOffset = 0;
  std::map<int, std::u16string> contentCache;

  FontAtlasImpl(const std::string &path, uint32_t fontSize) {
    if (FT_Init_FreeType(&ft)) {
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
                << std::endl;
      return;
    }
    readFont(path, fontSize);
  }

  void readFont(std::string path, uint32_t fontSize) {
    if (wasGenerated) {
      FT_Done_Face(face);
    }
    int x = FT_New_Face(ft, path.c_str(), 0, &face);
    if (x) {
      std::cout << "ERROR::FREETYPE: Failed to load font " << x << std::endl;
      return;
    }
    renderFont(fontSize);
  }

public:
  void renderFont(uint32_t fontSize) {
    if (wasGenerated) {
      for (std::map<char16_t, CharacterEntry>::iterator it = entries.begin();
           it != entries.end(); ++it) {
        delete[] it->second.data;
      }
      entries.clear();
      linesCache.clear();
    }
    fs = fontSize;
    atlas_width = 0;
    atlas_height = 0;
    smallest_top = 1e9;
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    // TODO should this be here?
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < 128; i++) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        std::cout << "Failed to load char: " << (char)i << "\n";
        return;
      }
      auto bm = face->glyph->bitmap;
      atlas_width += bm.width;
      atlas_height = bm.rows > atlas_height ? bm.rows : atlas_height;
    }
    atlas_width *= 2;

    // texture
    texture = Texture::create(atlas_width, atlas_height);

    xOffset = 0;
    for (int i = 0; i < 128; i++) {
      if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
        std::cout << "Failed to load char: " << (char)i << "\n";
        return;
      }

      CharacterEntry entry;
      auto bm = face->glyph->bitmap;
      entry.width = bm.width;
      entry.height = bm.rows;
      entry.top = face->glyph->bitmap_top;
      entry.left = face->glyph->bitmap_left;
      entry.advance = face->glyph->advance.x >> 6;
      entry.xPos = xOffset;
      entry.c = (char16_t)i;
      (&entry)->data = new uint8_t[(int)entry.width * (int)entry.height];
      memcpy(entry.data, face->glyph->bitmap.buffer,
             entry.width * entry.height);
      if (smallest_top == 0 && entry.top > 0)
        smallest_top = entry.top;
      else
        smallest_top = entry.top < smallest_top && entry.top != 0
                           ? entry.top
                           : smallest_top;
      entries.insert(std::pair<char16_t, CharacterEntry>(entry.c, entry));

      entries[(char16_t)i].offset = (float)xOffset / (float)atlas_width;
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, entry.width, entry.height,
                      GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

      xOffset += entry.width;
    }
    wasGenerated = true;
  }

private:
  void lazyLoad(char16_t c) {
    if (entries.count(c))
      return;
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cout << "Failed to load char: " << (char)c << "\n";
      return;
    }
    CharacterEntry entry;
    auto bm = face->glyph->bitmap;
    entry.width = bm.width;
    entry.height = bm.rows;
    entry.top = face->glyph->bitmap_top;
    entry.left = face->glyph->bitmap_left;
    entry.advance = face->glyph->advance.x >> 6;
    entry.xPos = xOffset;
    auto old_width = atlas_width;
    auto old_height = atlas_height;
    atlas_width += bm.width;
    atlas_height = bm.rows > atlas_height ? bm.rows : atlas_height;
    entry.offset = (float)xOffset / (float)atlas_width;
    GLuint new_tex_id;
    glGenTextures(1, &new_tex_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, new_tex_id);
    // params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei)atlas_width,
                 (GLsizei)atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    //   glCopyImageSubData(texture_id, GL_TEXTURE_2D, 0, 0, 0, 0, new_tex_id,
    //   GL_TEXTURE_2D, 0, 0, 0, 0, old_width, old_height, 0);
    //   glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,0,0, old_width, old_height);
    //   xOffset = 0;
    for (std::map<char16_t, CharacterEntry>::iterator it = entries.begin();
         it != entries.end(); ++it) {
      it->second.offset = (float)it->second.xPos / (float)atlas_width;
      glTexSubImage2D(GL_TEXTURE_2D, 0, it->second.xPos, 0, it->second.width,
                      it->second.height, GL_RED, GL_UNSIGNED_BYTE,
                      it->second.data);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, entry.width, entry.height,
                    GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
    entry.c = (char16_t)c;
    (&entry)->data = new uint8_t[(int)entry.width * (int)entry.height];
    memcpy(entry.data, face->glyph->bitmap.buffer, entry.width * entry.height);
    xOffset += entry.width;
    entries.insert(std::pair<char16_t, CharacterEntry>(entry.c, entry));
  }

public:
  RenderChar render(char16_t c, float x = 0.0, float y = 0.0,
                    Vec4f color = vec4fs(1)) {
    if (c >= 128)
      lazyLoad(c);

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
    : _impl(new FontAtlasImpl(path, fontSize)) {}
FontAtlas::~FontAtlas() { delete _impl; }
void FontAtlas::readFont(const std::string &path, uint32_t fontSize) {
  _impl->readFont(path, fontSize);
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
