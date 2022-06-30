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

public:
  bool _edited = false;
  bool _streamMode = false;
  bool _useXFallback;
  std::string _branch;
  std::vector<std::u16string> _lines;
  std::map<std::string, PosEntry> _saveLocs;
  std::deque<HistoryEntry> _history;
  std::filesystem::file_time_type _last_write_time;
  Selection _selection;
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

  void setBounds(float height, float lineHeight);
  void trimTrailingWhiteSpaces();
  void comment(std::u16string commentStr);
  void setRenderStart(float x, float y);
  void setPosFromMouse(float mouseX, float mouseY, class FontAtlas *atlas);
  void reset();
  void deleteSelection();
  std::string getSelection();
  int getSelectionSize();
  void bindTo(std::u16string *entry, bool useXSave = false);
  void unbind();
  std::u16string search(std::u16string what, bool skipFirst,
                        bool shouldOffset = true);
  std::u16string replaceOne(std::u16string what, std::u16string replace,
                            bool allowCenter = true, bool shouldOffset = true);
  size_t replaceAll(std::u16string what, std::u16string replace);
  int findAnyOf(std::u16string str, std::u16string what);
  int findAnyOfLast(std::u16string str, std::u16string what);
  void advanceWord();
  std::u16string deleteWord();
  bool undo();
  void advanceWordBackwards();
  void gotoLine(int l);
  void center(int l);
  std::vector<std::u16string> split(std::u16string base,
                                    std::u16string delimiter);
  std::vector<std::string> split(std::string base, std::string delimiter);

  void historyPush(int mode, int length, std::u16string content);
  void historyPush(int mode, int length, std::u16string content,
                   void *userData);
  void historyPushWithExtra(int mode, int length, std::u16string content,
                            std::vector<std::u16string> extra);
  bool didChange(std::string path);
  bool reloadFile(std::string path);
  bool openFile(std::string oldPath, std::string path);
  void append(char16_t c);
  void appendWithLines(std::u16string content);
  void append(std::u16string content);
  std::u16string getCurrentAdvance(bool useSaveValue = false);
  void removeBeforeCursor();
  void removeOne();
  void moveUp();
  void moveDown();
  void jumpStart();
  void jumpEnd();
  void moveRight();
  void moveLeft();
  bool saveTo(std::string path);
  std::vector<std::pair<int, std::u16string>> *
  getContent(class FontAtlas *atlas, float maxWidth, bool onlyCalculate);
  void moveLine(int diff);
  void calcTotalOffset();
  int getTotalOffset();
  std::vector<std::string> getSaveLocKeys();
};
