#include <glad.h>
#include "drawable.h"
#include "vertex_buffer.h"
#include "shader.h"
#include <memory>
#include <vector>
#include <stdint.h>

struct DrawableImpl {

  DrawableImpl(const DrawableImpl &) = delete;
  DrawableImpl &operator=(const DrawableImpl &) = delete;
  std::shared_ptr<Shader> shader;
  VBO vbo;
  VAO vao;

  DrawableImpl(const std::shared_ptr<Shader> &shader, size_t stride,
               const VertexLayout *layouts, size_t len, size_t dataSize)
      : shader(shader) {

    vbo.dynamicData(dataSize);

    vao.bind();
    vbo.bind();
    for (size_t i = 0; i < len; ++i, ++layouts) {
      glEnableVertexAttribArray(i);
      glVertexAttribPointer(i, layouts->size, GL_FLOAT, GL_FALSE, stride,
                            (void *)layouts->offset);
      glVertexAttribDivisor(i, layouts->divisor);
    }
    vao.unbind();
    vbo.unbind();
  }

  void drawTriangleStrip(int count) { vao.drawTriangleStrip(count); }

  void drawUploadInstance(const void *data, size_t len, int count,
                          int instance) {
    vbo.upload(data, len);
    vao.drawTriangleStripInstance(count, instance);
  }
};

Drawable::Drawable(const std::shared_ptr<Shader> &shader,
                   size_t stride, const VertexLayout *layouts, size_t len,
                   size_t dataSize)
    : _impl(new DrawableImpl(shader, stride, layouts, len,
                             dataSize)) {}
Drawable::~Drawable() { delete _impl; }

void Drawable::use() { _impl->shader->use(); }
void Drawable::set(const std::string &name, float x, float y) {
  _impl->shader->set2f(name, x, y);
}
void Drawable::set(const std::string &name, float x, float y, float z,
                   float w) {
  _impl->shader->set4f(name, x, y, z, w);
}
void Drawable::drawTriangleStrip(int count) { _impl->drawTriangleStrip(count); }
void Drawable::drawUploadInstance(const void *data, size_t len, int count,
                                  int instance) {
  _impl->drawUploadInstance(data, len, count, instance);
}
