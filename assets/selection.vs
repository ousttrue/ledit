#version 330 core
uniform vec2 resolution;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
vec2 camera_project(vec2 point) { return 2 * (point) * (1 / resolution); }

void main() {
  vec2 uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));

  gl_Position = vec4(camera_project(uv * size + position), 0.0f, 1.0f);
}
