#pragma once
#include <stdint.h>
#include "vertex_buffer.h"

struct VertexLayout {
  uint32_t type; // GL_FLOAT
  int size;      // 1, 2, 3, 4(scalar, vec2, vec3, vec4)
  size_t offset;
  uint32_t divisor;
};

class Drawable {

  Drawable(const Drawable &) = delete;
  Drawable &operator=(const Drawable &) = delete;
  Shader shader;
  VBO vbo;
  VAO vao;

public:
  Drawable(const std::string &vertex, const std::string &fragment,
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
