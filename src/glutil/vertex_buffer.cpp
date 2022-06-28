#include "glad.h"
#include "vertex_buffer.h"

///
/// VBO
///
VBO::VBO() { glGenBuffers(1, &handle); }

VBO::~VBO() { glDeleteBuffers(1, &handle); }

void VBO::bind() { glBindBuffer(GL_ARRAY_BUFFER, handle); }

void VBO::unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void VBO::dynamicData(size_t size) {
  bind();
  glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  unbind();
}

void VBO::upload(const void *data, size_t size) {
  bind();
  // be sure to use glBufferSubData and not glBufferData
  glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
  unbind();
}

///
/// VAO
///

VAO::VAO() { glGenVertexArrays(1, &handle); }

VAO::~VAO() { glDeleteVertexArrays(1, &handle); }

void VAO::bind() { glBindVertexArray(handle); }

void VAO::unbind() { glBindVertexArray(0); }

void VAO::drawTriangleStrip(uint32_t count) {
  bind();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, count);
  unbind();
}

void VAO::drawTriangleStripInstance(uint32_t count, uint32_t instance) {
  bind();
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, count, instance);
  unbind();
}


