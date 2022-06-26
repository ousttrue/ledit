#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include "highlighting.h"
#include "providers.h"

struct ReplaceBuffer {
  std::u16string search = u"";
  std::u16string replace = u"";
};

class State {

public:
  uint32_t vao, vbo;
  bool focused = true;
  bool exitFlag = false;
  bool cacheValid = false;
  uint32_t sel_vao, sel_vbo;
  uint32_t highlight_vao, highlight_vbo;
  class Cursor *cursor;
  std::vector<class CursorEntry *> cursors;
  size_t activeIndex;
  Highlighter highlighter;
  Provider provider;
  class FontAtlas *atlas;
  class GLFWwindow *window;
  ReplaceBuffer replaceBuffer;
  float WIDTH, HEIGHT;
  bool hasHighlighting;
  bool ctrlPressed = false;
  std::string path;
  std::u16string fileName;
  std::u16string status;
  std::u16string miniBuf;
  std::u16string dummyBuf;
  bool showLineNumbers = true;
  bool highlightLine = true;
  int mode = 0;
  int round = 0;
  int fontSize;
  State() {}
  State(float w, float h, int fontSize) {

    this->fontSize = fontSize;
    WIDTH = w;
    HEIGHT = h;
  }

  void resize(float w, float h);
  void focus(bool focused);
  void invalidateCache() { cacheValid = false; }

  class CursorEntry *hasEditedBuffer();
  void startReplace();
  void tryComment();
  void checkChanged();
  void switchMode();
  void increaseFontSize(int value);
  void toggleSelection();
  void switchBuffer();
  void tryPaste();
  void cut();
  void tryCopy();
  void save();
  void saveNew();
  void changeFont();
  void open();
  void reHighlight();
  void undo();
  void search();
  void tryEnableHighlighting();
  void inform(bool success, bool shift_pressed);
  void provideComplete(bool reverse);
  void renderCoords();
  void gotoLine();
  std::u16string getTabInfo();

  void deleteActive() { deleteCursor(activeIndex); }
  void rotateBuffer();
  void deleteCursor(size_t index);
  void activateCursor(size_t cursorIndex);
  void addCursor(std::string path);
  void init();

private:
  void activate_entries();
};
