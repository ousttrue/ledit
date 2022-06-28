#include "drawable.h"
#include "vertex_buffer.h"
#include "shader.h"

class DrawableImpl {

  DrawableImpl(const DrawableImpl &) = delete;
  DrawableImpl &operator=(const DrawableImpl &) = delete;
  Shader shader;
  VBO vbo;
  VAO vao;

public:
  DrawableImpl(const std::string &vertex, const std::string &fragment,
               const std::vector<std::string> &others, size_t stride,
               const VertexLayout *layouts, size_t len, size_t dataSize)
      : shader(vertex, fragment, others) {

    vbo.dynamicData(dataSize);

    vao.bind();
    vbo.bind();
    for (size_t i = 0; i < len; ++i, ++layouts) {
      glEnableVertexAttribArray(i);
      glVertexAttribPointer(i, layouts->size, layouts->type, GL_FALSE, stride,
                            (void *)layouts->offset);
      glVertexAttribDivisor(i, layouts->divisor);
    }
    vao.unbind();
    vbo.unbind();
  }

  void use() { shader.use(); }
  void set(const std::string &name, const Vec2f &value) {
    shader.set2f(name, value.x, value.y);
  }
  void set(const std::string &name, const Vec4f &value) {
    shader.set4f(name, value.x, value.y, value.z, value.w);
  }

  void drawTriangleStrip(int count) { vao.drawTriangleStrip(count); }

  void drawUploadInstance(const void *data, size_t len, int count,
                          int instance) {
    vbo.upload(data, len);
    vao.drawTriangleStripInstance(count, instance);
  }
};

Drawable::Drawable(const std::string &vertex, const std::string &fragment,
                   const std::vector<std::string> &others, size_t stride,
                   const VertexLayout *layouts, size_t len, size_t dataSize)
    : _impl(new DrawableImpl(vertex, fragment, others, stride, layouts, len,
                             dataSize)) {}
Drawable::~Drawable() { delete _impl; }

void Drawable::use() { _impl->use(); }
void Drawable::set(const std::string &name, const Vec2f &value) {
  _impl->set(name, value);
}
void Drawable::set(const std::string &name, const Vec4f &value) {
  _impl->set(name, value);
}
void Drawable::drawTriangleStrip(int count) { _impl->drawTriangleStrip(count); }
void Drawable::drawUploadInstance(const void *data, size_t len, int count,
                                  int instance) {
  _impl->drawUploadInstance(data, len, count, instance);
}
