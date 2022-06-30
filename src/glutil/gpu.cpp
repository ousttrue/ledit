#include <glad.h>
#include "gpu.h"

namespace gpu {

bool initialize(void *getProc) {
  if (!gladLoadGLLoader((GLADloadproc)getProc)) {
    return false;
  }

  // OpenGL state
  // ------------
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return true;
}

void clear(int w, int h, const float color[4]) {
  glViewport(0, 0, w, h);
  glClearColor(color[0], color[1], color[2], color[3]);
  glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace gpu
