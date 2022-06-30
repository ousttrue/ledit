#include "document.h"
#include "font_atlas.h"
#include "u8String.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <fstream>

static std::vector<std::string> splitNewLine(const std::string &base) {
  std::stringstream stream;
  stream.str("");
  stream.clear();
  std::vector<std::string> final;
  for (auto c = base.begin(); c != base.end(); c++) {
    const char e = *c;
    if (e == '\n') {
      final.push_back(stream.str());
      stream.str("");
      stream.clear();
    } else {
      stream << e;
    }
  }
  final.push_back(stream.str());
  return final;
}

std::shared_ptr<Document> Document::open(const std::string &path)
{
  auto cursor = std::shared_ptr<Document>(new Document);
  cursor->setPath(path);
  if(path.empty())
  {
    return cursor;
  }

  if (path == "-") {
    cursor->_lines.clear();
    std::string line;
    while (std::getline(std::cin, line)) {
      cursor->_lines.push_back(create(line));
    }
    return cursor;
  }

  std::ifstream stream(path);
  if (!stream.is_open()) {    
    return cursor;
  }

  std::stringstream ss;
  ss << stream.rdbuf();
  std::string c = ss.str();
  auto parts = splitNewLine(c);

  cursor->_lines = std::vector<std::u16string>(parts.size());
  size_t count = 0;
  for (const auto &ref : parts) {
    cursor->_lines[count] = create(ref);
    count++;
  }
  stream.close();
  cursor->_last_write_time = std::filesystem::last_write_time(path);
  return cursor;
}

void Document::setBounds(float height, float lineHeight) {
  this->_height = height;
  this->_lineHeight = lineHeight;
  float next = floor(height / lineHeight);
  if (_maxLines != 0) {
    if (next < _maxLines) {
      _skip += _maxLines - next;
    }
  }
  _maxLines = next;
}

void Document::trimTrailingWhiteSpaces() {
  for (auto &line : _lines) {
    char16_t last = line[line.length() - 1];
    if (last == ' ' || last == '\t' || last == '\r') {
      int remaining = line.length();
      int count = 0;
      while (remaining--) {
        char16_t current = line[remaining];
        if (current == ' ' || current == '\t' || current == '\r')
          count++;
        else
          break;
      }
      line = line.substr(0, line.length() - count);
    }
  }
  if (_x > _lines[_y].length())
    _x = _lines[_y].length();
}

void Document::comment(std::u16string commentStr) {
  if (!_selection.active) {
    std::u16string firstLine = _lines[_y];
    int firstOffset = 0;
    for (char c : firstLine) {
      if (c != ' ' && c != '\t')
        break;
      firstOffset++;
    }
    bool remove = firstLine.length() - firstOffset >= commentStr.length() &&
                  firstLine.find(commentStr) == firstOffset;
    if (remove) {
      CommentEntry *cm = new CommentEntry();
      cm->commentStr = commentStr;
      (&_lines[_y])->erase(firstOffset, commentStr.length());
      historyPush(42, firstOffset, u"", cm);
    } else {
      CommentEntry *cm = new CommentEntry();
      cm->commentStr = commentStr;
      (&_lines[_y])->insert(firstOffset, commentStr);
      historyPush(43, firstOffset, u"", cm);
    }
    return;
  }
  int firstOffset = 0;
  int yStart = _selection.getYSmaller();
  int yEnd = _selection.getYBigger();
  std::u16string firstLine = _lines[yStart];
  for (char c : firstLine) {
    if (c != ' ' && c != '\t')
      break;
    firstOffset++;
  }
  bool remove = firstLine.length() - firstOffset >= commentStr.length() &&
                firstLine.find(commentStr) == firstOffset;
  CommentEntry *cm = new CommentEntry();
  cm->firstOffset = firstOffset;
  cm->commentStr = commentStr;
  cm->yStart = yStart;
  if (remove) {
    historyPush(40, 0, u"", cm);
    for (size_t i = yStart; i < yEnd; i++) {
      if ((&_lines[i])->find(commentStr) != firstOffset)
        break;
      (&_lines[i])->erase(firstOffset, commentStr.length());
      _history[_history.size() - 1].length += 1;
    }
  } else {
    historyPush(41, yEnd - yStart, u"", cm);
    for (size_t i = yStart; i < yEnd; i++) {
      (&_lines[i])->insert(firstOffset, commentStr);
    }
  }
  _selection.stop();
}

void Document::setRenderStart(float x, float y) {
  _startX = x;
  _startY = y;
}

void Document::setPosFromMouse(float mouseX, float mouseY, FontAtlas *atlas) {
  if (_bind != nullptr)
    return;
  if (mouseY < _startY)
    return;
  int targetY = floor((mouseY - _startY) / _lineHeight);
  if (_skip + targetY >= _lines.size())
    targetY = _lines.size() - 1;
  else
    targetY += _skip;
  auto *line = &_lines[targetY];
  int targetX = 0;
  if (mouseX > _startX) {
    mouseX -= _startX;
    auto *advances = atlas->getAllAdvance(*line, targetY);
    float acc = 0;
    for (auto &entry : *advances) {
      acc += entry;
      if (acc > mouseX)
        break;
      targetX++;
    }
  }
  _x = targetX;
  _y = targetY;
  _selection.diffX(_x);
  _selection.diffY(_y);
}

void Document::reset() {
  _x = 0;
  _y = 0;
  _xSave = 0;
  _skip = 0;
  _prepare.clear();
  _history.clear();
  _lines = {u""};
}

void Document::deleteSelection() {
  if (_selection.yStart == _selection.yEnd) {
    auto line = _lines[_y];
    historyPush(16, line.length(), line);
    auto start = line.substr(0, _selection.getXSmaller());
    auto end = line.substr(_selection.getXBigger());
    _lines[_y] = start + end;
    _x = start.length();
  } else {
    int ySmall = _selection.getYSmaller();
    int yBig = _selection.getYBigger();
    bool isStart = ySmall == _selection.yStart;
    std::u16string save = _lines[ySmall];
    std::vector<std::u16string> toSave;
    _lines[ySmall] =
        _lines[ySmall].substr(0, isStart ? _selection.xStart : _selection.xEnd);
    for (int i = 0; i < yBig - ySmall; i++) {
      toSave.push_back(_lines[ySmall + 1]);
      if (i == yBig - ySmall - 1) {
        _x = _lines[ySmall].length();
        _lines[ySmall] += _lines[ySmall + 1].substr(isStart ? _selection.xEnd
                                                          : _selection.xStart);
      }
      _lines.erase(_lines.begin() + ySmall + 1);
    }
    _y = ySmall;
    historyPushWithExtra(16, save.length(), save, toSave);
  }
}

std::string Document::getSelection() {
  std::stringstream ss;
  if (_selection.yStart == _selection.yEnd) {

    ss << convert_str(_lines[_selection.yStart].substr(
        _selection.getXSmaller(),
        _selection.getXBigger() - _selection.getXSmaller()));
  } else {
    int ySmall = _selection.getYSmaller();
    int yBig = _selection.getYBigger();
    bool isStart = ySmall == _selection.yStart;
    ss << convert_str(
        _lines[ySmall].substr(isStart ? _selection.xStart : _selection.xEnd));
    ss << "\n";
    for (int i = ySmall + 1; i < yBig; i++) {
      ss << convert_str(_lines[i]);
      if (i != yBig)
        ss << "\n";
    }
    ss << convert_str(
        _lines[yBig].substr(0, isStart ? _selection.xEnd : _selection.xStart));
  }
  return ss.str();
}

int Document::getSelectionSize() {
  if (!_selection.active)
    return 0;
  if (_selection.yStart == _selection.yEnd)
    return _selection.getXBigger() - _selection.getXSmaller();
  int offset =
      (_lines[_selection.yStart].length() - _selection.xStart) + _selection.xEnd;
  for (int w = _selection.getYSmaller(); w < _selection.getYBigger(); w++) {
    if (w == _selection.getYSmaller() || w == _selection.getYBigger()) {
      continue;
    }
    offset += _lines[w].length() + 1;
  }
  return offset;
}

void Document::bindTo(std::u16string *entry, bool useXSave) {
  _bind = entry;
  _xSave = _x;
  this->_useXFallback = useXSave;
  _x = entry->length();
}

void Document::unbind() {
  _bind = nullptr;
  _useXFallback = false;
  _x = _xSave;
}

std::u16string Document::search(std::u16string what, bool skipFirst,
                              bool shouldOffset) {
  int i = shouldOffset ? _y : 0;
  bool found = false;
  for (int x = i; x < _lines.size(); x++) {
    auto line = _lines[x];
    auto where = line.find(what);
    if (where != std::string::npos) {
      if (skipFirst && !found) {
        found = true;
        continue;
      }
      _y = x;
      // we are in non 0 mode here, set savex
      _xSave = where;
      center(i);
      return u"[At: " + numberToString(_y + 1) + u":" +
             numberToString(where + 1) + u"]: ";
    }
    i++;
  }
  if (skipFirst)
    return u"[No further matches]: ";
  return u"[Not found]: ";
}

std::u16string Document::replaceOne(std::u16string what, std::u16string replace,
                                  bool allowCenter, bool shouldOffset) {
  int i = shouldOffset ? _y : 0;
  bool found = false;
  for (int x = i; x < _lines.size(); x++) {
    auto line = _lines[x];
    auto where = line.find(what, _xSave);
    if (where != std::string::npos) {
      auto xNow = this->_x;
      auto yNow = this->_y;
      this->_y = x;
      this->_x = where;
      historyPush(30, line.length(), line);
      std::u16string base = line.substr(0, where);
      base += replace;
      if (line.length() - where - what.length() > 0)
        base += line.substr(where + what.length());
      _lines[x] = base;
      if (allowCenter) {
        this->_y = i;
        center(i);
      } else {
        this->_y = yNow;
      }
      _xSave = where + replace.length();
      this->_x = xNow;
      return u"[At: " + numberToString(_y + 1) + u":" +
             numberToString(where + 1) + u"]: ";
    }
    i++;
    if (x < _lines.size() - 1)
      _xSave = 0;
  }
  return u"[Not found]: ";
}

size_t Document::replaceAll(std::u16string what, std::u16string replace) {
  size_t c = 0;
  while (true) {
    auto res = replaceOne(what, replace, false);
    if (res == u"[Not found]: ")
      break;
    c++;
  }
  historyPush(31, c, u"");
  if (_x > _lines[_y].length()) {
    _x = _lines[_y].length();
    _xSave = _x;
  }
  return c;
}

int Document::findAnyOf(std::u16string str, std::u16string what) {
  if (str.length() == 0)
    return -1;
  std::u16string::const_iterator c;
  int offset = 0;
  for (c = str.begin(); c != str.end(); c++) {

    if (c != str.begin() && what.find(*c) != std::string::npos) {
      return offset;
    }
    offset++;
  }

  return -1;
}

int Document::findAnyOfLast(std::u16string str, std::u16string what) {
  if (str.length() == 0)
    return -1;
  std::u16string::const_iterator c;
  int offset = 0;
  for (c = str.end() - 1; c != str.begin(); c--) {

    if (c != str.end() - 1 && what.find(*c) != std::string::npos) {
      return offset;
    }
    offset++;
  }

  return -1;
}

void Document::advanceWord() {
  std::u16string *target = _bind ? _bind : &_lines[_y];
  int offset = findAnyOf(target->substr(_x), wordSeperator);
  if (offset == -1) {
    if (_x == target->length() && _y < _lines.size() - 1) {
      _x = 0;
      _y++;
    } else {
      _x = target->length();
    }
  } else {
    _x += offset;
  }
  _selection.diffX(_x);
  _selection.diffY(_y);
}

std::u16string Document::deleteWord() {
  std::u16string *target = _bind ? _bind : &_lines[_y];
  int offset = findAnyOf(target->substr(_x), wordSeperator);
  if (offset == -1)
    offset = target->length() - _x;
  std::u16string w = target->substr(_x, offset);
  target->erase(_x, offset);
  historyPush(3, w.length(), w);
  return w;
}

bool Document::undo() {
  if (_history.size() == 0)
    return false;
  HistoryEntry entry = _history[0];
  _history.pop_front();
  switch (entry.mode) {
  case 3: {
    _x = entry.x;
    _y = entry.y;
    center(_y);
    (&_lines[_y])->insert(_x, entry.content);
    _x += entry.length;
    break;
  }
  case 4: {
    _y = entry.y;
    _x = entry.x;
    center(_y);
    (&_lines[_y])->insert(_x - 1, entry.content);
    break;
  }
  case 5: {
    _y = entry.y;
    _x = 0;
    _lines.insert(_lines.begin() + _y, entry.content);
    center(_y);
    if (entry.extra.size())
      _lines[_y - 1] = entry.extra[0];
    break;
  }
  case 6: {
    _y = entry.y;
    _x = (&_lines[_y])->length();
    _lines.erase(_lines.begin() + _y + 1);
    center(_y);
    break;
  }
  case 7: {
    _y = entry.y;
    _x = 0;
    if (entry.extra.size()) {
      _lines[_y] = entry.content + entry.extra[0];
      _lines.erase(_lines.begin() + _y + 1);
    } else {
      _lines.erase(_lines.begin() + _y);
    }
    center(_y);
    break;
  }
  case 8: {
    _y = entry.y;
    _x = entry.x;
    (&_lines[_y])->erase(_x, 1);
    center(_y);
    break;
  }
  case 10: {
    _x = 0;
    _y = entry.y;
    _lines[_y] = entry.content;
    _lines.insert(_lines.begin() + _y, u"");
    center(_y);
    break;
  }
  case 11: {
    _y = entry.y;
    _x = entry.x;
    (&_lines[_y])->insert(_x, entry.content);
    break;
  }
  case 15: {
    if (entry.length == 0) {
      _y = entry.y;
      _x = entry.x;
      (&_lines[_y])->erase(_x, entry.content.length());
    } else {
      _y = entry.y - entry.length;
      _x = entry.x;
      for (size_t i = 0; i < entry.length; i++) {
        _lines.erase(_lines.begin() + _y + 1);
      }
      _lines[_y] = entry.content;
    }
    break;
  }
  case 16: {
    if (entry.extra.size()) {
      _y = entry.y;
      _x = entry.x;
      _lines[_y] = entry.content;
      for (int i = 0; i < entry.extra.size(); i++) {
        _lines.insert(_lines.begin() + _y + i + 1, entry.extra[i]);
      }

    } else {
      _y = entry.y;
      _x = entry.x;
      _lines[_y] = entry.content;
    }
    break;
  }
  case 30: {
    _y = entry.y;
    _x = entry.x;
    _lines[_y] = entry.content;
    break;
  }
  case 31: {
    for (size_t i = 0; i < entry.length; i++) {
      undo();
    }
    _y = entry.y;
    _x = entry.x;
    break;
  }
  case 40: {
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    std::u16string commentStr = data->commentStr;
    size_t len = entry.length;
    for (size_t i = data->yStart; i < data->yStart + len; i++) {
      (&_lines[i])->insert(data->firstOffset, commentStr);
    }
    _x = data->firstOffset;
    _y = data->yStart;
    center(_y);
    delete data;
    break;
  }
  case 41: {
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    std::u16string commentStr = data->commentStr;
    size_t len = entry.length;
    for (size_t i = data->yStart; i < data->yStart + len; i++) {
      (&_lines[i])->erase(data->firstOffset, commentStr.length());
    }
    _x = data->firstOffset;
    _y = data->yStart;
    center(_y);
    delete data;
    break;
  }
  case 42: {
    _y = entry.y;
    center(_y);
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    (&_lines[_y])->insert(entry.length, data->commentStr);
    _x = entry.x;
    delete data;
    break;
  }
  case 43: {
    _y = entry.y;
    center(_y);
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    (&_lines[_y])->erase(entry.length, data->commentStr.length());
    _x = entry.x;
    delete data;
    break;
  }
  default:
    return false;
  }
  return true;
}

void Document::advanceWordBackwards() {
  std::u16string *target = _bind ? _bind : &_lines[_y];
  int offset = findAnyOfLast(target->substr(0, _x), wordSeperator);
  if (offset == -1) {
    if (_x == 0 && _y > 0) {
      _y--;
      _x = _lines[_y].length();
    } else {
      _x = 0;
    }
  } else {
    _x -= offset;
  }
  _selection.diffX(_x);
  _selection.diffY(_y);
}

void Document::gotoLine(int l) {
  if (l - 1 > _lines.size())
    return;
  _x = 0;
  _xSave = 0;
  _y = l - 1;
  _selection.diff(_x, _y);
  center(l);
}

void Document::center(int l) {
  if (l >= _skip && l <= _skip + _maxLines)
    return;
  if (l < _maxLines / 2 || _lines.size() < l)
    _skip = 0;
  else {
    if (_lines.size() - l < _maxLines / 2)
      _skip = _lines.size() - _maxLines;
    else
      _skip = l - (_maxLines / 2);
  }
}

std::vector<std::u16string> Document::split(std::u16string base,
                                          std::u16string delimiter) {
  std::vector<std::u16string> final;
  size_t pos = 0;
  std::u16string token;
  while ((pos = base.find(delimiter)) != std::string::npos) {
    token = base.substr(0, pos);
    final.push_back(token);
    base.erase(0, pos + delimiter.length());
  }
  final.push_back(base);
  return final;
}

std::vector<std::string> Document::split(std::string base,
                                       std::string delimiter) {
  std::vector<std::string> final;
  final.reserve(base.length() / 76);
  size_t pos = 0;
  std::string token;
  while ((pos = base.find(delimiter)) != std::string::npos) {
    token = base.substr(0, pos);
    final.push_back(token);
    base.erase(0, pos + delimiter.length());
  }
  final.push_back(base);
  return final;
}

void Document::historyPush(int mode, int length, std::u16string content) {
  if (_bind != nullptr)
    return;
  _edited = true;
  HistoryEntry entry;
  entry.x = _x;
  entry.y = _y;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  if (_history.size() > 5000)
    _history.pop_back();
  _history.push_front(entry);
}

void Document::historyPush(int mode, int length, std::u16string content,
                         void *userData) {
  if (_bind != nullptr)
    return;
  _edited = true;
  HistoryEntry entry;
  entry.x = _x;
  entry.y = _y;
  entry.userData = userData;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  if (_history.size() > 5000)
    _history.pop_back();
  _history.push_front(entry);
}

void Document::historyPushWithExtra(int mode, int length, std::u16string content,
                                  std::vector<std::u16string> extra) {
  if (_bind != nullptr)
    return;
  _edited = true;
  HistoryEntry entry;
  entry.x = _x;
  entry.y = _y;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  entry.extra = extra;
  if (_history.size() > 5000)
    _history.pop_back();
  _history.push_front(entry);
}

bool Document::didChange(std::string path) {
  if (!std::filesystem::exists(path))
    return false;
  bool result = _last_write_time != std::filesystem::last_write_time(path);
  _last_write_time = std::filesystem::last_write_time(path);
  return result;
}

bool Document::reloadFile(std::string path) {
  std::ifstream stream(path);
  if (!stream.is_open())
    return false;
  _history.clear();
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string c = ss.str();
  auto parts = splitNewLine(c);
  _lines = std::vector<std::u16string>(parts.size());
  size_t count = 0;
  for (const auto &ref : parts) {
    _lines[count] = create(ref);
    count++;
  }
  if (_skip > _lines.size() - _maxLines)
    _skip = 0;
  if (_y > _lines.size() - 1)
    _y = _lines.size() - 1;
  if (_x > _lines[_y].length())
    _x = _lines[_y].length();
  stream.close();
  _last_write_time = std::filesystem::last_write_time(path);
  _edited = false;
  return true;
}

bool Document::openFile(std::string oldPath, std::string path) {
  std::ifstream stream(path);
  if (oldPath.length()) {
    PosEntry entry;
    entry.x = _xSave;
    entry.y = _y;
    entry.skip = _skip;
    _saveLocs[oldPath] = entry;
  }

  if (!stream.is_open()) {
    return false;
  }
  if (_saveLocs.count(path)) {
    PosEntry savedEntry = _saveLocs[path];
    _x = savedEntry.x;
    _y = savedEntry.y;
    _skip = savedEntry.skip;
  } else {
    {
      // this is purely for the buffer switcher
      PosEntry idk{0, 0, 0};
      _saveLocs[path] = idk;
    }
    _x = 0;
    _y = 0;
    _skip = 0;
  }
  _xSave = _x;
  _history.clear();
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string c = ss.str();
  auto parts = splitNewLine(c);
  _lines = std::vector<std::u16string>(parts.size());
  size_t count = 0;
  for (const auto &ref : parts) {
    _lines[count] = create(ref);
    count++;
  }
  if (_skip > _lines.size() - _maxLines)
    _skip = 0;
  if (_y > _lines.size() - 1)
    _y = _lines.size() - 1;
  if (_x > _lines[_y].length())
    _x = _lines[_y].length();
  stream.close();
  _last_write_time = std::filesystem::last_write_time(path);
  _edited = false;
  return true;
}

void Document::append(char16_t c) {
  if (_selection.active) {
    deleteSelection();
    _selection.stop();
  }
  if (c == '\n' && _bind == nullptr) {
    auto pos = _lines.begin() + _y;
    std::u16string *current = &_lines[_y];
    bool isEnd = _x == current->length();
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
      _lines.insert(pos + 1, base);
      historyPush(6, 0, u"");
      _x = base.length();
      _y++;
      return;

    } else {
      if (_x == 0) {
        _lines.insert(pos, u"");
        historyPush(7, 0, u"");
      } else {
        std::u16string toWrite = current->substr(0, _x);
        std::u16string next = current->substr(_x);
        _lines[_y] = toWrite;
        _lines.insert(pos + 1, next);
        historyPushWithExtra(7, toWrite.length(), toWrite, {next});
      }
    }
    _y++;
    _x = 0;
  } else {
    auto *target = _bind ? _bind : &_lines[_y];
    std::u16string content;
    content += c;
    target->insert(_x, content);
    historyPush(8, 1, content);
    _x++;
  }
}

void Document::appendWithLines(std::u16string content) {
  if (_bind) {
    append(content);
    return;
  }
  if (_selection.active) {
    deleteSelection();
    _selection.stop();
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
        (&_lines[_y])->insert(_x, contentLines[i]);
        historyPush(15, 0, contentLines[i]);
      } else {
        historySave = _lines[_y];
        hasSave = true;
        save = _lines[_y].substr(_x);
        _lines[_y] = _lines[_y].substr(0, _x) + contentLines[i];
        saveX = _x;
      }
      _x += contentLines[i].length();
      continue;
    } else if (i == contentLines.size() - 1) {
      _lines.insert(_lines.begin() + _y + 1, contentLines[i]);
      _y++;
      count++;

      _x = contentLines[i].length();
    } else {
      _lines.insert(_lines.begin() + _y + 1, contentLines[i]);
      _y++;
      count++;
    }
  }
  if (hasSave) {
    _lines[_y] += save;
    int xx = _x;
    _x = saveX;
    historyPush(15, count, historySave);
    _x = xx;
  }
  center(_y);
}

void Document::append(std::u16string content) {
  auto *target = _bind ? _bind : &_lines[_y];
  target->insert(_x, content);
  _x += content.length();
}

std::u16string Document::getCurrentAdvance(bool useSaveValue) {
  if (useSaveValue)
    return _lines[_y].substr(0, _xSave);

  if (_bind)
    return _bind->substr(0, _x);
  return _lines[_y].substr(0, _x);
}

void Document::removeBeforeCursor() {
  if (_selection.active)
    return;
  std::u16string *target = _bind ? _bind : &_lines[_y];
  if (_x == 0 && target->length() == 0) {
    if (_y == _lines.size() - 1 || _bind)
      return;
    if (target->length() == 0) {
      std::u16string next = _lines[_y + 1];
      _lines[_y] = next;
      _lines.erase(_lines.begin() + _y + 1);
      historyPush(10, next.length(), next);
      return;
    }
  }
  historyPush(11, 1, std::u16string(1, (*target)[_x]));
  target->erase(_x, 1);

  if (_x > target->length())
    _x = target->length();
}

void Document::removeOne() {
  if (_selection.active) {
    deleteSelection();
    _selection.stop();
    return;
  }
  std::u16string *target = _bind ? _bind : &_lines[_y];
  if (_x == 0) {
    if (_y == 0 || _bind)
      return;

    std::u16string *copyTarget = &_lines[_y - 1];
    int xTarget = copyTarget->length();
    if (target->length() > 0) {
      historyPushWithExtra(5, (&_lines[_y])->length(), _lines[_y], {_lines[_y - 1]});
      copyTarget->append(*target);
    } else {
      historyPush(5, (&_lines[_y])->length(), _lines[_y]);
    }
    _lines.erase(_lines.begin() + _y);

    _y--;
    _x = xTarget;
  } else {
    historyPush(4, 1, std::u16string(1, (*target)[_x - 1]));
    target->erase(_x - 1, 1);
    _x--;
  }
}

void Document::moveUp() {
  if (_y == 0 || _bind)
    return;
  std::u16string *target = &_lines[_y - 1];
  int targetX = target->length() < _x ? target->length() : _x;
  _x = targetX;
  _y--;
  _selection.diff(_x, _y);
}

void Document::moveDown() {
  if (_bind || _y == _lines.size() - 1)
    return;
  std::u16string *target = &_lines[_y + 1];
  int targetX = target->length() < _x ? target->length() : _x;
  _x = targetX;
  _y++;
  _selection.diff(_x, _y);
}

void Document::jumpStart() {
  _x = 0;
  _selection.diffX(_x);
}

void Document::jumpEnd() {
  if (_bind)
    _x = _bind->length();
  else
    _x = _lines[_y].length();
  _selection.diffX(_x);
}

void Document::moveRight() {
  std::u16string *current = _bind ? _bind : &_lines[_y];
  if (_x == current->length()) {
    if (_y == _lines.size() - 1 || _bind)
      return;
    _y++;
    _x = 0;
  } else {
    _x++;
  }
  _selection.diff(_x, _y);
}

void Document::moveLeft() {
  std::u16string *current = _bind ? _bind : &_lines[_y];
  if (_x == 0) {
    if (_y == 0 || _bind)
      return;
    std::u16string *target = &_lines[_y - 1];
    _y--;
    _x = target->length();
  } else {
    _x--;
  }
  _selection.diff(_x, _y);
}

bool Document::saveTo(std::string path) {
  if (!hasEnding(path, ".md"))
    trimTrailingWhiteSpaces();
  if (path == "-") {
    auto &stream = std::cout;
    for (size_t i = 0; i < _lines.size(); i++) {
      stream << convert_str(_lines[i]);
      if (i < _lines.size() - 1)
        stream << "\n";
    }
    exit(0);
    return true;
  }
  std::ofstream stream(path, std::ofstream::out);
  if (!stream.is_open()) {
    return false;
  }
  for (size_t i = 0; i < _lines.size(); i++) {
    stream << convert_str(_lines[i]);
    if (i < _lines.size() - 1)
      stream << "\n";
  }
  stream.flush();
  stream.close();
  _last_write_time = std::filesystem::last_write_time(path);
  _edited = false;
  return true;
}

std::vector<std::pair<int, std::u16string>> *
Document::getContent(FontAtlas *atlas, float maxWidth, bool onlyCalculate) {
  _prepare.clear();
  int end = _skip + _maxLines;
  if (end >= _lines.size()) {
    end = _lines.size();
    _skip = end - _maxLines;
    if (_skip < 0)
      _skip = 0;
  }
  if (_y == end && end < (_lines.size())) {
    _skip++;
    end++;

  } else if (_y < _skip && _skip > 0) {
    _skip--;
    end--;
  }
  /*
    Here we only care about the frame to render being adjusted for the
    linenumbers, content itself relies on maxWidth being accurate!
  */
  if (onlyCalculate)
    return nullptr;
  int maxSupport = 0;
  for (size_t i = _skip; i < end; i++) {
    auto s = _lines[i];
    _prepare.push_back(std::pair<int, std::u16string>(s.length(), s));
  }
  float neededAdvance =
      atlas->getAdvance((&_lines[_y])->substr(0, _useXFallback ? _xSave : _x));
  int xOffset = 0;
  if (neededAdvance > maxWidth) {
    auto *all = atlas->getAllAdvance(_lines[_y], _y - _skip);
    auto len = _lines[_y].length();
    float acc = 0;
    _xSkip = 0;
    for (auto value : *all) {
      if (acc > neededAdvance) {

        break;
      }
      if (acc > maxWidth * 2) {
        xOffset++;
        _xSkip += value;
      }
      acc += value;
    }
  } else {
    _xSkip = 0;
  }
  if (xOffset > 0) {
    for (size_t i = 0; i < _prepare.size(); i++) {
      auto a = _prepare[i].second;
      if (a.length() > xOffset)
        _prepare[i].second = a.substr(xOffset);
      else
        _prepare[i].second = u"";
    }
  }
  this->_xOffset = xOffset;
  return &_prepare;
}

void Document::moveLine(int diff) {
  int targetY = _y + diff;
  if (targetY < 0 || targetY == _lines.size())
    return;
  if (targetY < _y) {
    std::u16string toOffset = _lines[_y - 1];
    _lines[_y - 1] = _lines[_y];
    _lines[_y] = toOffset;
  } else {
    std::u16string toOffset = _lines[_y + 1];
    _lines[_y + 1] = _lines[_y];
    _lines[_y] = toOffset;
  }
  _y = targetY;
}

void Document::calcTotalOffset() {
  int offset = 0;
  for (int i = 0; i < _skip; i++) {
    offset += _lines[i].length() + 1;
  }
  _totalCharOffset = offset;
}

int Document::getTotalOffset() {
  if (_cachedY != _y || _cachedX != _x || _cachedMaxLines != _maxLines) {
    calcTotalOffset();
    _cachedMaxLines = _maxLines;
    _cachedY = _y;
    _cachedX = _x;
  }
  return _totalCharOffset;
}

std::vector<std::string> Document::getSaveLocKeys() {
  std::vector<std::string> ls;
  for (std::map<std::string, PosEntry>::iterator it = _saveLocs.begin();
       it != _saveLocs.end(); ++it) {
    ls.push_back(it->first);
  }
  return ls;
}
