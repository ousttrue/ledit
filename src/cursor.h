#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include "font_atlas.h"
#include "selection.h"
#include <deque>
#include "u8String.h"
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

class Cursor {
public:
  bool edited = false;
  bool streamMode = false;
  bool useXFallback;
  std::string branch;
  std::vector<std::u16string> lines;
  std::map<std::string, PosEntry> saveLocs;
  std::deque<HistoryEntry> history;
  std::filesystem::file_time_type last_write_time;
  Selection selection;
  int x = 0;
  int y = 0;
  int xSave = 0;
  int skip = 0;
  int xOffset = 0;
  float xSkip = 0;
  float height = 0;
  float lineHeight = 0;
  int maxLines = 0;
  int totalCharOffset = 0;
  int cachedY = 0;
  int cachedX = 0;
  int cachedMaxLines = 0;

  float startX = 0;
  float startY = 0;
  std::vector<std::pair<int, std::u16string>> prepare;
  std::u16string *bind = nullptr;

  Cursor() { lines.push_back(u""); }
  Cursor(std::string path);
  void setBounds(float height, float lineHeight);
  void trimTrailingWhiteSpaces();
  void comment(std::u16string commentStr);
  void setRenderStart(float x, float y);
  void setPosFromMouse(float mouseX, float mouseY, FontAtlas *atlas);
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
  std::vector<std::string> splitNewLine(std::string *base);
  void historyPush(int mode, int length, std::u16string content);
  void historyPush(int mode, int length, std::u16string content,
                   void *userData);
  void historyPushWithExtra(int mode, int length, std::u16string content,
                            std::vector<std::u16string> extra);
  bool didChange(std::string path) {
    if (!std::filesystem::exists(path))
      return false;
    bool result = last_write_time != std::filesystem::last_write_time(path);
    last_write_time = std::filesystem::last_write_time(path);
    return result;
  }
  bool reloadFile(std::string path) {
    std::ifstream stream(path);
    if (!stream.is_open())
      return false;
    history.clear();
    std::stringstream ss;
    ss << stream.rdbuf();
    std::string c = ss.str();
    auto parts = splitNewLine(&c);
    lines = std::vector<std::u16string>(parts.size());
    size_t count = 0;
    for (const auto &ref : parts) {
      lines[count] = create(ref);
      count++;
    }
    if (skip > lines.size() - maxLines)
      skip = 0;
    if (y > lines.size() - 1)
      y = lines.size() - 1;
    if (x > lines[y].length())
      x = lines[y].length();
    stream.close();
    last_write_time = std::filesystem::last_write_time(path);
    edited = false;
    return true;
  }
  bool openFile(std::string oldPath, std::string path) {
    std::ifstream stream(path);
    if (oldPath.length()) {
      PosEntry entry;
      entry.x = xSave;
      entry.y = y;
      entry.skip = skip;
      saveLocs[oldPath] = entry;
    }

    if (!stream.is_open()) {
      return false;
    }
    if (saveLocs.count(path)) {
      PosEntry savedEntry = saveLocs[path];
      x = savedEntry.x;
      y = savedEntry.y;
      skip = savedEntry.skip;
    } else {
      {
        // this is purely for the buffer switcher
        PosEntry idk{0, 0, 0};
        saveLocs[path] = idk;
      }
      x = 0;
      y = 0;
      skip = 0;
    }
    xSave = x;
    history.clear();
    std::stringstream ss;
    ss << stream.rdbuf();
    std::string c = ss.str();
    auto parts = splitNewLine(&c);
    lines = std::vector<std::u16string>(parts.size());
    size_t count = 0;
    for (const auto &ref : parts) {
      lines[count] = create(ref);
      count++;
    }
    if (skip > lines.size() - maxLines)
      skip = 0;
    if (y > lines.size() - 1)
      y = lines.size() - 1;
    if (x > lines[y].length())
      x = lines[y].length();
    stream.close();
    last_write_time = std::filesystem::last_write_time(path);
    edited = false;
    return true;
  }
  void append(char16_t c) {
    if (selection.active) {
      deleteSelection();
      selection.stop();
    }
    if (c == '\n' && bind == nullptr) {
      auto pos = lines.begin() + y;
      std::u16string *current = &lines[y];
      bool isEnd = x == current->length();
      if (isEnd) {
        std::u16string base;
        for (size_t t = 0; t < current->length(); t++) {
          if ((*current)[t] == ' ')
            base += u" ";
          else if ((*current)[t] == '\t')
            base += u"\t";
          else
            break;
        }
        lines.insert(pos + 1, base);
        historyPush(6, 0, u"");
        x = base.length();
        y++;
        return;

      } else {
        if (x == 0) {
          lines.insert(pos, u"");
          historyPush(7, 0, u"");
        } else {
          std::u16string toWrite = current->substr(0, x);
          std::u16string next = current->substr(x);
          lines[y] = toWrite;
          lines.insert(pos + 1, next);
          historyPushWithExtra(7, toWrite.length(), toWrite, {next});
        }
      }
      y++;
      x = 0;
    } else {
      auto *target = bind ? bind : &lines[y];
      std::u16string content;
      content += c;
      target->insert(x, content);
      historyPush(8, 1, content);
      x++;
    }
  }
  void appendWithLines(std::u16string content) {
    if (bind) {
      append(content);
      return;
    }
    if (selection.active) {
      deleteSelection();
      selection.stop();
    }
    bool hasSave = false;
    std::u16string save;
    std::u16string historySave;
    auto contentLines = split(content, u"\n");
    int saveX = 0;
    int count = 0;
    for (int i = 0; i < contentLines.size(); i++) {
      if (i == 0) {
        if (contentLines.size() == 1) {
          (&lines[y])->insert(x, contentLines[i]);
          historyPush(15, 0, contentLines[i]);
        } else {
          historySave = lines[y];
          hasSave = true;
          save = lines[y].substr(x);
          lines[y] = lines[y].substr(0, x) + contentLines[i];
          saveX = x;
        }
        x += contentLines[i].length();
        continue;
      } else if (i == contentLines.size() - 1) {
        lines.insert(lines.begin() + y + 1, contentLines[i]);
        y++;
        count++;

        x = contentLines[i].length();
      } else {
        lines.insert(lines.begin() + y + 1, contentLines[i]);
        y++;
        count++;
      }
    }
    if (hasSave) {
      lines[y] += save;
      int xx = x;
      x = saveX;
      historyPush(15, count, historySave);
      x = xx;
    }
    center(y);
  }
  void append(std::u16string content) {
    auto *target = bind ? bind : &lines[y];
    target->insert(x, content);
    x += content.length();
  }

  std::u16string getCurrentAdvance(bool useSaveValue = false) {
    if (useSaveValue)
      return lines[y].substr(0, xSave);

    if (bind)
      return bind->substr(0, x);
    return lines[y].substr(0, x);
  }
  void removeBeforeCursor() {
    if (selection.active)
      return;
    std::u16string *target = bind ? bind : &lines[y];
    if (x == 0 && target->length() == 0) {
      if (y == lines.size() - 1 || bind)
        return;
      if (target->length() == 0) {
        std::u16string next = lines[y + 1];
        lines[y] = next;
        lines.erase(lines.begin() + y + 1);
        historyPush(10, next.length(), next);
        return;
      }
    }
    historyPush(11, 1, std::u16string(1, (*target)[x]));
    target->erase(x, 1);

    if (x > target->length())
      x = target->length();
  }
  void removeOne() {
    if (selection.active) {
      deleteSelection();
      selection.stop();
      return;
    }
    std::u16string *target = bind ? bind : &lines[y];
    if (x == 0) {
      if (y == 0 || bind)
        return;

      std::u16string *copyTarget = &lines[y - 1];
      int xTarget = copyTarget->length();
      if (target->length() > 0) {
        historyPushWithExtra(5, (&lines[y])->length(), lines[y],
                             {lines[y - 1]});
        copyTarget->append(*target);
      } else {
        historyPush(5, (&lines[y])->length(), lines[y]);
      }
      lines.erase(lines.begin() + y);

      y--;
      x = xTarget;
    } else {
      historyPush(4, 1, std::u16string(1, (*target)[x - 1]));
      target->erase(x - 1, 1);
      x--;
    }
  }
  void moveUp() {
    if (y == 0 || bind)
      return;
    std::u16string *target = &lines[y - 1];
    int targetX = target->length() < x ? target->length() : x;
    x = targetX;
    y--;
    selection.diff(x, y);
  }
  void moveDown() {
    if (bind || y == lines.size() - 1)
      return;
    std::u16string *target = &lines[y + 1];
    int targetX = target->length() < x ? target->length() : x;
    x = targetX;
    y++;
    selection.diff(x, y);
  }

  void jumpStart() {
    x = 0;
    selection.diffX(x);
  }

  void jumpEnd() {
    if (bind)
      x = bind->length();
    else
      x = lines[y].length();
    selection.diffX(x);
  }

  void moveRight() {
    std::u16string *current = bind ? bind : &lines[y];
    if (x == current->length()) {
      if (y == lines.size() - 1 || bind)
        return;
      y++;
      x = 0;
    } else {
      x++;
    }
    selection.diff(x, y);
  }
  void moveLeft() {
    std::u16string *current = bind ? bind : &lines[y];
    if (x == 0) {
      if (y == 0 || bind)
        return;
      std::u16string *target = &lines[y - 1];
      y--;
      x = target->length();
    } else {
      x--;
    }
    selection.diff(x, y);
  }
  bool saveTo(std::string path) {
    if (!hasEnding(path, ".md"))
      trimTrailingWhiteSpaces();
    if (path == "-") {
      auto &stream = std::cout;
      for (size_t i = 0; i < lines.size(); i++) {
        stream << convert_str(lines[i]);
        if (i < lines.size() - 1)
          stream << "\n";
      }
      exit(0);
      return true;
    }
    std::ofstream stream(path, std::ofstream::out);
    if (!stream.is_open()) {
      return false;
    }
    for (size_t i = 0; i < lines.size(); i++) {
      stream << convert_str(lines[i]);
      if (i < lines.size() - 1)
        stream << "\n";
    }
    stream.flush();
    stream.close();
    last_write_time = std::filesystem::last_write_time(path);
    edited = false;
    return true;
  }
  std::vector<std::pair<int, std::u16string>> *
  getContent(FontAtlas *atlas, float maxWidth, bool onlyCalculate) {
    prepare.clear();
    int end = skip + maxLines;
    if (end >= lines.size()) {
      end = lines.size();
      skip = end - maxLines;
      if (skip < 0)
        skip = 0;
    }
    if (y == end && end < (lines.size())) {
      skip++;
      end++;

    } else if (y < skip && skip > 0) {
      skip--;
      end--;
    }
    /*
      Here we only care about the frame to render being adjusted for the
      linenumbers, content itself relies on maxWidth being accurate!
    */
    if (onlyCalculate)
      return nullptr;
    int maxSupport = 0;
    for (size_t i = skip; i < end; i++) {
      auto s = lines[i];
      prepare.push_back(std::pair<int, std::u16string>(s.length(), s));
    }
    float neededAdvance =
        atlas->getAdvance((&lines[y])->substr(0, useXFallback ? xSave : x));
    int xOffset = 0;
    if (neededAdvance > maxWidth) {
      auto *all = atlas->getAllAdvance(lines[y], y - skip);
      auto len = lines[y].length();
      float acc = 0;
      xSkip = 0;
      for (auto value : *all) {
        if (acc > neededAdvance) {

          break;
        }
        if (acc > maxWidth * 2) {
          xOffset++;
          xSkip += value;
        }
        acc += value;
      }
    } else {
      xSkip = 0;
    }
    if (xOffset > 0) {
      for (size_t i = 0; i < prepare.size(); i++) {
        auto a = prepare[i].second;
        if (a.length() > xOffset)
          prepare[i].second = a.substr(xOffset);
        else
          prepare[i].second = u"";
      }
    }
    this->xOffset = xOffset;
    return &prepare;
  }
  void moveLine(int diff) {
    int targetY = y + diff;
    if (targetY < 0 || targetY == lines.size())
      return;
    if (targetY < y) {
      std::u16string toOffset = lines[y - 1];
      lines[y - 1] = lines[y];
      lines[y] = toOffset;
    } else {
      std::u16string toOffset = lines[y + 1];
      lines[y + 1] = lines[y];
      lines[y] = toOffset;
    }
    y = targetY;
  }
  void calcTotalOffset() {
    int offset = 0;
    for (int i = 0; i < skip; i++) {
      offset += lines[i].length() + 1;
    }
    totalCharOffset = offset;
  }
  int getTotalOffset() {
    if (cachedY != y || cachedX != x || cachedMaxLines != maxLines) {
      calcTotalOffset();
      cachedMaxLines = maxLines;
      cachedY = y;
      cachedX = x;
    }
    return totalCharOffset;
  }
  std::vector<std::string> getSaveLocKeys() {
    std::vector<std::string> ls;
    for (std::map<std::string, PosEntry>::iterator it = saveLocs.begin();
         it != saveLocs.end(); ++it) {
      ls.push_back(it->first);
    }
    return ls;
  }
};

struct CursorEntry {
  Cursor cursor;
  std::string path;
};
