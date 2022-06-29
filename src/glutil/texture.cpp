#include <glad.h>
#include <memory>
#include "texture.h"

struct TextureImpl {
  GLuint handle = 0;

  TextureImpl(int w, int h) {
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    // params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE,
                 nullptr);
  }

  ~TextureImpl() { glDeleteTextures(1, &handle); }
};

///
/// Texture
///
Texture::Texture(int w, int h) : _impl(new TextureImpl(w, h)) {}

Texture::~Texture() { delete _impl; }

std::shared_ptr<Texture> Texture::create(int w, int h) {
  auto p = std::shared_ptr<Texture>(new Texture(w, h));
  return p;
}

uint32_t Texture::getHandle() const { return _impl->handle; }
