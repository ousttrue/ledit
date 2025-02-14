#pragma once
#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include "highlighting.h"
#include "config_provider.h"
#include "document.h"

struct ReplaceBuffer {
  std::u16string search = u"";
  std::u16string replace = u"";
};

class State {

public:
  bool focused = true;
  bool exitFlag = false;
  bool cacheValid = false;
  std::shared_ptr<Document> active;
  std::vector<std::shared_ptr<Document>> cursors;
  Highlighter highlighter;
  Provider provider;
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

  State() {}
  State(float w, float h) {
    WIDTH = w;
    HEIGHT = h;
  }

  void resize(float w, float h);
  void focus(bool focused);
  void invalidateCache() { cacheValid = false; }

  std::shared_ptr<Document> hasEditedBuffer() const;
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

  size_t getActiveIndex() const {
    size_t i = 0;
    for (; i < cursors.size(); ++i) {
      if (cursors[i] == active) {
        break;
      }
    }
    return i;
  }
  void deleteActive() { deleteCursor(active); }
  void rotateBuffer();
  void deleteCursor(const std::shared_ptr<Document> &cursor);
  void activateCursor(size_t cursorIndex);
  void addCursor(std::string path);  
};
