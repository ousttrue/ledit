#include "glad.h"
#include "vertex_buffer.h"

VBO::VBO() { glGenBuffers(1, &handle); }

VBO::~VBO() {}

void VBO::bind() { glBindBuffer(GL_ARRAY_BUFFER, handle); }

void VBO::unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void VBO::dynamicData(size_t size) {
  glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

void VBO::upload(const void *data, size_t size) {
  bind();
  // be sure to use glBufferSubData and not glBufferData
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
  unbind();
}
