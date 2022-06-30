#pragma once

#include "selection.h"
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <memory>
#ifndef __APPLE__
#include <filesystem>
#endif

struct PosEntry {
  int x, y, skip;
};

struct HistoryEntry {
  int x, y;
  int mode;
  int length;
  void *userData;
  std::u16string content;
  std::vector<std::u16string> extra;
};

struct CommentEntry {
  int firstOffset;
  int yStart;
  std::u16string commentStr;
};

class Document {
  std::string _path;
  bool _streamMode = false;
  bool _useXFallback;
  std::map<std::string, PosEntry> _saveLocs;
  std::filesystem::file_time_type _last_write_time;

public:
  std::string _branch;
  bool _edited = false;
  std::vector<std::u16string> _lines;
  Selection _selection;
  std::deque<HistoryEntry> _history;

  int _x = 0;
  int _y = 0;
  int _xSave = 0;
  int _skip = 0;
  int _xOffset = 0;
  float _xSkip = 0;
  float _height = 0;
  float _lineHeight = 0;
  int _maxLines = 0;
  int _totalCharOffset = 0;
  int _cachedY = 0;
  int _cachedX = 0;
  int _cachedMaxLines = 0;

  float _startX = 0;
  float _startY = 0;
  std::vector<std::pair<int, std::u16string>> _prepare;
  std::u16string *_bind = nullptr;

  Document() { _lines.push_back(u""); }
  static std::shared_ptr<Document> open(const std::string &path);

  std::string getPath() const { return _path; }
  void setPath(const std::string &path) { _path = path; }
  void comment(std::u16string commentStr);
  void setBounds(float height, float lineHeight);
  void setRenderStart(float x, float y);
  std::string getSelection();
  int getSelectionSize();

private:
  void trimTrailingWhiteSpaces();
  void setPosFromMouse(float mouseX, float mouseY, class FontAtlas *atlas);
  void reset();
  void deleteSelection();

public:
  void advanceWordBackwards();
  std::u16string replaceOne(std::u16string what, std::u16string replace,
                            bool allowCenter = true, bool shouldOffset = true);
  size_t replaceAll(std::u16string what, std::u16string replace);
  void bindTo(std::u16string *entry, bool useXSave = false);
  void unbind();
  std::u16string search(std::u16string what, bool skipFirst,
                        bool shouldOffset = true);
  std::u16string deleteWord();
  bool undo();
  void gotoLine(int l);
  bool didChange(std::string path);
  bool reloadFile(std::string path);
  void advanceWord();

private:
  void center(int l);

  void historyPush(int mode, int length, std::u16string content);
  void historyPush(int mode, int length, std::u16string content,
                   void *userData);
  void historyPushWithExtra(int mode, int length, std::u16string content,
                            std::vector<std::u16string> extra);
  bool openFile(std::string oldPath, std::string path);
  void appendWithLines(std::u16string content);

public:
  void removeOne();
  void removeBeforeCursor();
  void moveUp();
  void moveDown();
  void append(char16_t c);
  void append(std::u16string content);
  std::u16string getCurrentAdvance(bool useSaveValue = false);
  void jumpStart();
  void jumpEnd();
  bool saveTo(std::string path);
  std::vector<std::pair<int, std::u16string>> *
  getContent(class FontAtlas *atlas, float maxWidth, bool onlyCalculate);
  int getTotalOffset();
  void moveLine(int diff);
  void moveRight();
  void moveLeft();

private:
  void calcTotalOffset();
  std::vector<std::string> getSaveLocKeys();
};
