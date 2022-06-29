#include "cursor.h"
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

std::shared_ptr<Cursor> Cursor::open(const std::string &path)
{
  auto cursor = std::shared_ptr<Cursor>(new Cursor);
  cursor->setPath(path);
  if(path.empty())
  {
    return cursor;
  }

  if (path == "-") {
    cursor->lines.clear();
    std::string line;
    while (std::getline(std::cin, line)) {
      cursor->lines.push_back(create(line));
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

  cursor->lines = std::vector<std::u16string>(parts.size());
  size_t count = 0;
  for (const auto &ref : parts) {
    cursor->lines[count] = create(ref);
    count++;
  }
  stream.close();
  cursor->last_write_time = std::filesystem::last_write_time(path);
  return cursor;
}

void Cursor::setBounds(float height, float lineHeight) {
  this->height = height;
  this->lineHeight = lineHeight;
  float next = floor(height / lineHeight);
  if (maxLines != 0) {
    if (next < maxLines) {
      skip += maxLines - next;
    }
  }
  maxLines = next;
}

void Cursor::trimTrailingWhiteSpaces() {
  for (auto &line : lines) {
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
  if (x > lines[y].length())
    x = lines[y].length();
}

void Cursor::comment(std::u16string commentStr) {
  if (!selection.active) {
    std::u16string firstLine = lines[y];
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
      (&lines[y])->erase(firstOffset, commentStr.length());
      historyPush(42, firstOffset, u"", cm);
    } else {
      CommentEntry *cm = new CommentEntry();
      cm->commentStr = commentStr;
      (&lines[y])->insert(firstOffset, commentStr);
      historyPush(43, firstOffset, u"", cm);
    }
    return;
  }
  int firstOffset = 0;
  int yStart = selection.getYSmaller();
  int yEnd = selection.getYBigger();
  std::u16string firstLine = lines[yStart];
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
      if ((&lines[i])->find(commentStr) != firstOffset)
        break;
      (&lines[i])->erase(firstOffset, commentStr.length());
      history[history.size() - 1].length += 1;
    }
  } else {
    historyPush(41, yEnd - yStart, u"", cm);
    for (size_t i = yStart; i < yEnd; i++) {
      (&lines[i])->insert(firstOffset, commentStr);
    }
  }
  selection.stop();
}

void Cursor::setRenderStart(float x, float y) {
  startX = x;
  startY = y;
}

void Cursor::setPosFromMouse(float mouseX, float mouseY, FontAtlas *atlas) {
  if (bind != nullptr)
    return;
  if (mouseY < startY)
    return;
  int targetY = floor((mouseY - startY) / lineHeight);
  if (skip + targetY >= lines.size())
    targetY = lines.size() - 1;
  else
    targetY += skip;
  auto *line = &lines[targetY];
  int targetX = 0;
  if (mouseX > startX) {
    mouseX -= startX;
    auto *advances = atlas->getAllAdvance(*line, targetY);
    float acc = 0;
    for (auto &entry : *advances) {
      acc += entry;
      if (acc > mouseX)
        break;
      targetX++;
    }
  }
  x = targetX;
  y = targetY;
  selection.diffX(x);
  selection.diffY(y);
}

void Cursor::reset() {
  x = 0;
  y = 0;
  xSave = 0;
  skip = 0;
  prepare.clear();
  history.clear();
  lines = {u""};
}

void Cursor::deleteSelection() {
  if (selection.yStart == selection.yEnd) {
    auto line = lines[y];
    historyPush(16, line.length(), line);
    auto start = line.substr(0, selection.getXSmaller());
    auto end = line.substr(selection.getXBigger());
    lines[y] = start + end;
    x = start.length();
  } else {
    int ySmall = selection.getYSmaller();
    int yBig = selection.getYBigger();
    bool isStart = ySmall == selection.yStart;
    std::u16string save = lines[ySmall];
    std::vector<std::u16string> toSave;
    lines[ySmall] =
        lines[ySmall].substr(0, isStart ? selection.xStart : selection.xEnd);
    for (int i = 0; i < yBig - ySmall; i++) {
      toSave.push_back(lines[ySmall + 1]);
      if (i == yBig - ySmall - 1) {
        x = lines[ySmall].length();
        lines[ySmall] += lines[ySmall + 1].substr(isStart ? selection.xEnd
                                                          : selection.xStart);
      }
      lines.erase(lines.begin() + ySmall + 1);
    }
    y = ySmall;
    historyPushWithExtra(16, save.length(), save, toSave);
  }
}

std::string Cursor::getSelection() {
  std::stringstream ss;
  if (selection.yStart == selection.yEnd) {

    ss << convert_str(lines[selection.yStart].substr(
        selection.getXSmaller(),
        selection.getXBigger() - selection.getXSmaller()));
  } else {
    int ySmall = selection.getYSmaller();
    int yBig = selection.getYBigger();
    bool isStart = ySmall == selection.yStart;
    ss << convert_str(
        lines[ySmall].substr(isStart ? selection.xStart : selection.xEnd));
    ss << "\n";
    for (int i = ySmall + 1; i < yBig; i++) {
      ss << convert_str(lines[i]);
      if (i != yBig)
        ss << "\n";
    }
    ss << convert_str(
        lines[yBig].substr(0, isStart ? selection.xEnd : selection.xStart));
  }
  return ss.str();
}

int Cursor::getSelectionSize() {
  if (!selection.active)
    return 0;
  if (selection.yStart == selection.yEnd)
    return selection.getXBigger() - selection.getXSmaller();
  int offset =
      (lines[selection.yStart].length() - selection.xStart) + selection.xEnd;
  for (int w = selection.getYSmaller(); w < selection.getYBigger(); w++) {
    if (w == selection.getYSmaller() || w == selection.getYBigger()) {
      continue;
    }
    offset += lines[w].length() + 1;
  }
  return offset;
}

void Cursor::bindTo(std::u16string *entry, bool useXSave) {
  bind = entry;
  xSave = x;
  this->useXFallback = useXSave;
  x = entry->length();
}

void Cursor::unbind() {
  bind = nullptr;
  useXFallback = false;
  x = xSave;
}

std::u16string Cursor::search(std::u16string what, bool skipFirst,
                              bool shouldOffset) {
  int i = shouldOffset ? y : 0;
  bool found = false;
  for (int x = i; x < lines.size(); x++) {
    auto line = lines[x];
    auto where = line.find(what);
    if (where != std::string::npos) {
      if (skipFirst && !found) {
        found = true;
        continue;
      }
      y = x;
      // we are in non 0 mode here, set savex
      xSave = where;
      center(i);
      return u"[At: " + numberToString(y + 1) + u":" +
             numberToString(where + 1) + u"]: ";
    }
    i++;
  }
  if (skipFirst)
    return u"[No further matches]: ";
  return u"[Not found]: ";
}

std::u16string Cursor::replaceOne(std::u16string what, std::u16string replace,
                                  bool allowCenter, bool shouldOffset) {
  int i = shouldOffset ? y : 0;
  bool found = false;
  for (int x = i; x < lines.size(); x++) {
    auto line = lines[x];
    auto where = line.find(what, xSave);
    if (where != std::string::npos) {
      auto xNow = this->x;
      auto yNow = this->y;
      this->y = x;
      this->x = where;
      historyPush(30, line.length(), line);
      std::u16string base = line.substr(0, where);
      base += replace;
      if (line.length() - where - what.length() > 0)
        base += line.substr(where + what.length());
      lines[x] = base;
      if (allowCenter) {
        this->y = i;
        center(i);
      } else {
        this->y = yNow;
      }
      xSave = where + replace.length();
      this->x = xNow;
      return u"[At: " + numberToString(y + 1) + u":" +
             numberToString(where + 1) + u"]: ";
    }
    i++;
    if (x < lines.size() - 1)
      xSave = 0;
  }
  return u"[Not found]: ";
}

size_t Cursor::replaceAll(std::u16string what, std::u16string replace) {
  size_t c = 0;
  while (true) {
    auto res = replaceOne(what, replace, false);
    if (res == u"[Not found]: ")
      break;
    c++;
  }
  historyPush(31, c, u"");
  if (x > lines[y].length()) {
    x = lines[y].length();
    xSave = x;
  }
  return c;
}

int Cursor::findAnyOf(std::u16string str, std::u16string what) {
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

int Cursor::findAnyOfLast(std::u16string str, std::u16string what) {
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

void Cursor::advanceWord() {
  std::u16string *target = bind ? bind : &lines[y];
  int offset = findAnyOf(target->substr(x), wordSeperator);
  if (offset == -1) {
    if (x == target->length() && y < lines.size() - 1) {
      x = 0;
      y++;
    } else {
      x = target->length();
    }
  } else {
    x += offset;
  }
  selection.diffX(x);
  selection.diffY(y);
}

std::u16string Cursor::deleteWord() {
  std::u16string *target = bind ? bind : &lines[y];
  int offset = findAnyOf(target->substr(x), wordSeperator);
  if (offset == -1)
    offset = target->length() - x;
  std::u16string w = target->substr(x, offset);
  target->erase(x, offset);
  historyPush(3, w.length(), w);
  return w;
}

bool Cursor::undo() {
  if (history.size() == 0)
    return false;
  HistoryEntry entry = history[0];
  history.pop_front();
  switch (entry.mode) {
  case 3: {
    x = entry.x;
    y = entry.y;
    center(y);
    (&lines[y])->insert(x, entry.content);
    x += entry.length;
    break;
  }
  case 4: {
    y = entry.y;
    x = entry.x;
    center(y);
    (&lines[y])->insert(x - 1, entry.content);
    break;
  }
  case 5: {
    y = entry.y;
    x = 0;
    lines.insert(lines.begin() + y, entry.content);
    center(y);
    if (entry.extra.size())
      lines[y - 1] = entry.extra[0];
    break;
  }
  case 6: {
    y = entry.y;
    x = (&lines[y])->length();
    lines.erase(lines.begin() + y + 1);
    center(y);
    break;
  }
  case 7: {
    y = entry.y;
    x = 0;
    if (entry.extra.size()) {
      lines[y] = entry.content + entry.extra[0];
      lines.erase(lines.begin() + y + 1);
    } else {
      lines.erase(lines.begin() + y);
    }
    center(y);
    break;
  }
  case 8: {
    y = entry.y;
    x = entry.x;
    (&lines[y])->erase(x, 1);
    center(y);
    break;
  }
  case 10: {
    x = 0;
    y = entry.y;
    lines[y] = entry.content;
    lines.insert(lines.begin() + y, u"");
    center(y);
    break;
  }
  case 11: {
    y = entry.y;
    x = entry.x;
    (&lines[y])->insert(x, entry.content);
    break;
  }
  case 15: {
    if (entry.length == 0) {
      y = entry.y;
      x = entry.x;
      (&lines[y])->erase(x, entry.content.length());
    } else {
      y = entry.y - entry.length;
      x = entry.x;
      for (size_t i = 0; i < entry.length; i++) {
        lines.erase(lines.begin() + y + 1);
      }
      lines[y] = entry.content;
    }
    break;
  }
  case 16: {
    if (entry.extra.size()) {
      y = entry.y;
      x = entry.x;
      lines[y] = entry.content;
      for (int i = 0; i < entry.extra.size(); i++) {
        lines.insert(lines.begin() + y + i + 1, entry.extra[i]);
      }

    } else {
      y = entry.y;
      x = entry.x;
      lines[y] = entry.content;
    }
    break;
  }
  case 30: {
    y = entry.y;
    x = entry.x;
    lines[y] = entry.content;
    break;
  }
  case 31: {
    for (size_t i = 0; i < entry.length; i++) {
      undo();
    }
    y = entry.y;
    x = entry.x;
    break;
  }
  case 40: {
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    std::u16string commentStr = data->commentStr;
    size_t len = entry.length;
    for (size_t i = data->yStart; i < data->yStart + len; i++) {
      (&lines[i])->insert(data->firstOffset, commentStr);
    }
    x = data->firstOffset;
    y = data->yStart;
    center(y);
    delete data;
    break;
  }
  case 41: {
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    std::u16string commentStr = data->commentStr;
    size_t len = entry.length;
    for (size_t i = data->yStart; i < data->yStart + len; i++) {
      (&lines[i])->erase(data->firstOffset, commentStr.length());
    }
    x = data->firstOffset;
    y = data->yStart;
    center(y);
    delete data;
    break;
  }
  case 42: {
    y = entry.y;
    center(y);
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    (&lines[y])->insert(entry.length, data->commentStr);
    x = entry.x;
    delete data;
    break;
  }
  case 43: {
    y = entry.y;
    center(y);
    CommentEntry *data = static_cast<CommentEntry *>(entry.userData);
    (&lines[y])->erase(entry.length, data->commentStr.length());
    x = entry.x;
    delete data;
    break;
  }
  default:
    return false;
  }
  return true;
}

void Cursor::advanceWordBackwards() {
  std::u16string *target = bind ? bind : &lines[y];
  int offset = findAnyOfLast(target->substr(0, x), wordSeperator);
  if (offset == -1) {
    if (x == 0 && y > 0) {
      y--;
      x = lines[y].length();
    } else {
      x = 0;
    }
  } else {
    x -= offset;
  }
  selection.diffX(x);
  selection.diffY(y);
}

void Cursor::gotoLine(int l) {
  if (l - 1 > lines.size())
    return;
  x = 0;
  xSave = 0;
  y = l - 1;
  selection.diff(x, y);
  center(l);
}

void Cursor::center(int l) {
  if (l >= skip && l <= skip + maxLines)
    return;
  if (l < maxLines / 2 || lines.size() < l)
    skip = 0;
  else {
    if (lines.size() - l < maxLines / 2)
      skip = lines.size() - maxLines;
    else
      skip = l - (maxLines / 2);
  }
}

std::vector<std::u16string> Cursor::split(std::u16string base,
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

std::vector<std::string> Cursor::split(std::string base,
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

void Cursor::historyPush(int mode, int length, std::u16string content) {
  if (bind != nullptr)
    return;
  edited = true;
  HistoryEntry entry;
  entry.x = x;
  entry.y = y;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  if (history.size() > 5000)
    history.pop_back();
  history.push_front(entry);
}

void Cursor::historyPush(int mode, int length, std::u16string content,
                         void *userData) {
  if (bind != nullptr)
    return;
  edited = true;
  HistoryEntry entry;
  entry.x = x;
  entry.y = y;
  entry.userData = userData;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  if (history.size() > 5000)
    history.pop_back();
  history.push_front(entry);
}

void Cursor::historyPushWithExtra(int mode, int length, std::u16string content,
                                  std::vector<std::u16string> extra) {
  if (bind != nullptr)
    return;
  edited = true;
  HistoryEntry entry;
  entry.x = x;
  entry.y = y;
  entry.mode = mode;
  entry.length = length;
  entry.content = content;
  entry.extra = extra;
  if (history.size() > 5000)
    history.pop_back();
  history.push_front(entry);
}

bool Cursor::didChange(std::string path) {
  if (!std::filesystem::exists(path))
    return false;
  bool result = last_write_time != std::filesystem::last_write_time(path);
  last_write_time = std::filesystem::last_write_time(path);
  return result;
}

bool Cursor::reloadFile(std::string path) {
  std::ifstream stream(path);
  if (!stream.is_open())
    return false;
  history.clear();
  std::stringstream ss;
  ss << stream.rdbuf();
  std::string c = ss.str();
  auto parts = splitNewLine(c);
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

bool Cursor::openFile(std::string oldPath, std::string path) {
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
  auto parts = splitNewLine(c);
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

void Cursor::append(char16_t c) {
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

void Cursor::appendWithLines(std::u16string content) {
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

void Cursor::append(std::u16string content) {
  auto *target = bind ? bind : &lines[y];
  target->insert(x, content);
  x += content.length();
}

std::u16string Cursor::getCurrentAdvance(bool useSaveValue) {
  if (useSaveValue)
    return lines[y].substr(0, xSave);

  if (bind)
    return bind->substr(0, x);
  return lines[y].substr(0, x);
}

void Cursor::removeBeforeCursor() {
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

void Cursor::removeOne() {
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
      historyPushWithExtra(5, (&lines[y])->length(), lines[y], {lines[y - 1]});
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

void Cursor::moveUp() {
  if (y == 0 || bind)
    return;
  std::u16string *target = &lines[y - 1];
  int targetX = target->length() < x ? target->length() : x;
  x = targetX;
  y--;
  selection.diff(x, y);
}

void Cursor::moveDown() {
  if (bind || y == lines.size() - 1)
    return;
  std::u16string *target = &lines[y + 1];
  int targetX = target->length() < x ? target->length() : x;
  x = targetX;
  y++;
  selection.diff(x, y);
}

void Cursor::jumpStart() {
  x = 0;
  selection.diffX(x);
}

void Cursor::jumpEnd() {
  if (bind)
    x = bind->length();
  else
    x = lines[y].length();
  selection.diffX(x);
}

void Cursor::moveRight() {
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

void Cursor::moveLeft() {
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

bool Cursor::saveTo(std::string path) {
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
Cursor::getContent(FontAtlas *atlas, float maxWidth, bool onlyCalculate) {
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

void Cursor::moveLine(int diff) {
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

void Cursor::calcTotalOffset() {
  int offset = 0;
  for (int i = 0; i < skip; i++) {
    offset += lines[i].length() + 1;
  }
  totalCharOffset = offset;
}

int Cursor::getTotalOffset() {
  if (cachedY != y || cachedX != x || cachedMaxLines != maxLines) {
    calcTotalOffset();
    cachedMaxLines = maxLines;
    cachedY = y;
    cachedX = x;
  }
  return totalCharOffset;
}

std::vector<std::string> Cursor::getSaveLocKeys() {
  std::vector<std::string> ls;
  for (std::map<std::string, PosEntry>::iterator it = saveLocs.begin();
       it != saveLocs.end(); ++it) {
    ls.push_back(it->first);
  }
  return ls;
}
