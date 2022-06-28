#include <glad.h>
#include "glfwapp.h"
#include "state.h"
#include "cursor.h"
#include <GLFW/glfw3.h>

static State *getState(GLFWwindow *window) {
  return reinterpret_cast<State *>(glfwGetWindowUserPointer(window));
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  auto gState = getState(window);
#ifdef _WIN32
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  gState->resize((float)width * xscale, (float)height * yscale);
#else
  gState->resize((float)width, (float)height);
#endif
}

void window_focus_callback(GLFWwindow *window, int focused) {
  auto gState = getState(window);
  gState->focus(focused);
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  auto gState = getState(window);
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    gState->invalidateCache();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    gState->cursor->setPosFromMouse((float)xpos * xscale, (float)ypos * yscale,
                                    gState->atlas.get());
  }
}

void character_callback(GLFWwindow *window, unsigned int codepoint) {
  auto gState = getState(window);
  gState->invalidateCache();
  gState->exitFlag = false;
#ifdef _WIN32
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
  if (ctrl_pressed)
    return;
#endif
  bool alt_pressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
  if (alt_pressed) {
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
      gState->cursor->advanceWord();
      return;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      gState->tryCopy();
      return;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
      gState->cursor->advanceWordBackwards();
      return;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      gState->cursor->deleteWord();
      return;
    }
  }
  gState->cursor->append((char16_t)codepoint);
  gState->renderCoords();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  auto gState = getState(window);
  gState->invalidateCache();
  if (key == GLFW_KEY_ESCAPE) {
    if (action == GLFW_PRESS) {
      if (gState->cursor->selection.active) {
        gState->cursor->selection.stop();
        return;
      }
      if (gState->mode != 0) {
        gState->inform(false, false);
      } else {
        CursorEntry *edited = gState->hasEditedBuffer();
        if (gState->exitFlag || edited == nullptr) {
          glfwSetWindowShouldClose(window, true);
        } else {
          gState->exitFlag = true;
          gState->status =
              create(edited->path.length() ? edited->path : "New File") +
              u" edited, press ESC again to exit";
        }
      }
    }
    return;
  }
  gState->exitFlag = false;
  bool ctrl_pressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  gState->ctrlPressed = ctrl_pressed;
  bool shift_pressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  bool x_pressed = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
  bool alt_pressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
  Cursor *cursor = gState->cursor;
  bool isPress = action == GLFW_PRESS || action == GLFW_REPEAT;
#ifdef __linux__
  if (alt_pressed) {
    if (key == GLFW_KEY_F && isPress) {
      gState->cursor->advanceWord();
      return;
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
      gState->tryCopy();
      return;
    }
    if (key == GLFW_KEY_B && isPress) {
      gState->cursor->advanceWordBackwards();
      return;
    }
    if (key == GLFW_KEY_D && isPress) {
      gState->cursor->deleteWord();
      return;
    }
  }
#endif
  if (ctrl_pressed) {

    if (x_pressed) {
      if (action == GLFW_PRESS && key == GLFW_KEY_S) {
        gState->save();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_SLASH) {
        gState->tryComment();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_M) {
        gState->switchMode();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_L) {
        gState->showLineNumbers = !gState->showLineNumbers;
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_H) {
        gState->highlightLine = !gState->highlightLine;
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_O) {
        gState->open();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_0) {
        gState->changeFont();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_K) {
        gState->switchBuffer();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_N) {
        gState->saveNew();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_G) {
        gState->gotoLine();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_W) {
        gState->deleteActive();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_A) {
        cursor->gotoLine(1);
        gState->renderCoords();
      }
      if (action == GLFW_PRESS && key == GLFW_KEY_E) {
        cursor->gotoLine(cursor->lines.size());
        gState->renderCoords();
      }
      return;
    }
    if (shift_pressed) {
      if (key == GLFW_KEY_P && isPress) {
        gState->cursor->moveLine(-1);
      } else if (key == GLFW_KEY_N && isPress) {
        gState->cursor->moveLine(1);
      }
      gState->renderCoords();
      return;
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
      gState->search();
    } else if (key == GLFW_KEY_R && isPress) {
      gState->startReplace();
    } else if (key == GLFW_KEY_Z && isPress) {
      gState->undo();
    } else if (key == GLFW_KEY_W && isPress) {
      gState->cut();
    } else if (key == GLFW_KEY_SPACE && isPress) {
      gState->toggleSelection();
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
      gState->tryCopy();

    } else if (key == GLFW_KEY_EQUAL && isPress) {
      gState->increaseFontSize(2);
    } else if (key == GLFW_KEY_MINUS && isPress) {
      gState->increaseFontSize(-2);
    } else if ((key == GLFW_KEY_V || key == GLFW_KEY_Y) && isPress) {
      gState->tryPaste();
    } else {
      if (!isPress)
        return;
      if (key == GLFW_KEY_A && action == GLFW_PRESS)
        cursor->jumpStart();
      else if (key == GLFW_KEY_F && isPress)
        cursor->moveRight();
      else if (key == GLFW_KEY_D && isPress)
        cursor->removeBeforeCursor();
      else if (key == GLFW_KEY_E && isPress)
        cursor->jumpEnd();
      else if (key == GLFW_KEY_B && isPress)
        cursor->moveLeft();
      else if (key == GLFW_KEY_P && isPress)
        cursor->moveUp();
      else if (key == GLFW_KEY_N && isPress)
        cursor->moveDown();
      gState->renderCoords();
    }
  } else {
    if (isPress && key == GLFW_KEY_RIGHT)
      cursor->moveRight();
    if (isPress && key == GLFW_KEY_LEFT)
      cursor->moveLeft();
    if (isPress && key == GLFW_KEY_UP)
      cursor->moveUp();
    if (isPress && key == GLFW_KEY_DOWN)
      cursor->moveDown();
    if (isPress && key == GLFW_KEY_ENTER) {
      if (gState->mode != 0) {
        gState->inform(true, shift_pressed);
        return;
      } else
        cursor->append('\n');
    }
    if (isPress && key == GLFW_KEY_TAB) {
      if (gState->mode != 0)
        gState->provideComplete(shift_pressed);
      else
        cursor->append(u"  ");
    }
    if (isPress && key == GLFW_KEY_BACKSPACE) {
      cursor->removeOne();
    }
    if (isPress)
      gState->renderCoords();
  }
}

class GlfwAppImpl {
  GLFWwindow *_window = nullptr;

public:
  GlfwAppImpl() { glfwInit(); }

  ~GlfwAppImpl() {
    if (_window) {
      glfwDestroyWindow(_window);
    }
    glfwTerminate();
  }

  bool createWindow(const char *window_name, int width, int height,
                    void *userpointer, bool allowTransparency) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (allowTransparency)
      glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(width, height, window_name, nullptr, nullptr);
    if (!_window) {
      const char *description;
      int code = glfwGetError(&description);
      std::cout << "Failed to create GLFW _window: " << description
                << std::endl;
      return false;
    }

    glfwMakeContextCurrent(_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return false;
    }

    glfwSetWindowUserPointer(_window, userpointer);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
    glfwSetKeyCallback(_window, key_callback);
    glfwSetCharCallback(_window, character_callback);
    glfwSetMouseButtonCallback(_window, mouse_button_callback);
    glfwSetWindowFocusCallback(_window, window_focus_callback);
    GLFWcursor *mouseCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    glfwSetCursor(_window, mouseCursor);

    return true;
  }

  bool isAlive() { return !glfwWindowShouldClose(_window); }

  void swapBuffers() { glfwSwapBuffers(_window); }

  std::tuple<float, float> getScale() const {
    float xscale, yscale;
    glfwGetWindowContentScale(_window, &xscale, &yscale);
    return {xscale, yscale};
  }
};

//
// GlfwApp
//
GlfwApp::GlfwApp() : _impl(new GlfwAppImpl) {}

GlfwApp::~GlfwApp() { delete _impl; }

bool GlfwApp::createWindow(const char *title, int w, int h, void *userpointer,
                           bool allowTransparency) {
  return _impl->createWindow(title, w, h, userpointer, allowTransparency);
}

bool GlfwApp::isWindowAlive() { return _impl->isAlive(); }

std::tuple<float, float> GlfwApp::getScale() const { return _impl->getScale(); }

void GlfwApp::wait() { glfwWaitEvents(); }

void GlfwApp::flush() {
  _impl->swapBuffers();
  glfwWaitEvents();
}