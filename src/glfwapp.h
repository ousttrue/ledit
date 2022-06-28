#pragma once
#include <tuple>

class GlfwApp {
  class GlfwAppImpl *_impl = nullptr;

public:
  GlfwApp();
  ~GlfwApp();
  void *createWindow(const char *title, int w, int h, void *userpointer,
                     bool allowTransparency);
  bool isWindowAlive();
  void flush();
  std::tuple<float, float> getScale() const;
  void wait();
};
