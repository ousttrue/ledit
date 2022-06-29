#include <glad.h>
#include "state.h"
#include "glutil/shader.h"
#include "cursor.h"
#include "font_atlas.h"
#include "languages.h"
#include <GLFW/glfw3.h>

void State::resize(float w, float h) {
  glViewport(0, 0, w, h);
  invalidateCache();
  WIDTH = w;
  HEIGHT = h;
}

void State::focus(bool focused) {
  invalidateCache();
  this->focused = focused;
  if (focused) {
    checkChanged();
  }
}

std::shared_ptr<Cursor> State::hasEditedBuffer() const {
  for (auto &cursor : cursors) {
    if (active->edited)
      return cursor;
  }
  return {};
}

void State::startReplace() {
  if (mode != 0)
    return;
  mode = 30;
  status = u"Search: ";
  miniBuf = replaceBuffer.search;
  active->bindTo(&miniBuf);
}

void State::tryComment() {
  if (!hasHighlighting)
    return;
  active->comment(highlighter.language.singleLineComment);
}

void State::checkChanged() {
  if (!path.length())
    return;
  active->branch = provider.getBranchName(path);
  auto changed = active->didChange(path);
  if (changed) {
    miniBuf = u"";
    mode = 36;
    status = u"[" + create(path) + u"]: Changed on disk, reload?";
    active->bindTo(&dummyBuf);
  }
}

void State::switchMode() {
  if (mode != 0)
    return;
  round = 0;
  miniBuf = u"Text";
  status = u"Mode: ";
  active->bindTo(&dummyBuf);
  mode = 25;
}

void State::increaseFontSize(int value) {
  if (mode != 0)
    return;
  fontSize += value;
  if (fontSize > 260) {
    fontSize = 260;
    status = u"Max font size reached [260]";
    return;
  } else if (fontSize < 10) {
    fontSize = 10;
    status = u"Min font size reached [10]";
    return;
  } else {
    status = u"resize: [" + numberToString(fontSize) + u"]";
  }
  atlas->renderFont(fontSize);
}

void State::toggleSelection() {
  if (mode != 0)
    return;
  if (active->selection.active)
    active->selection.stop();
  else
    active->selection.activate(active->x, active->y);
  renderCoords();
}

void State::switchBuffer() {
  if (mode != 0 && mode != 5)
    return;
  if (mode == 0) {
    if (cursors.size() == 1) {
      status = u"No other buffers in cache";
      return;
    }
    round = 0;
    miniBuf = create(cursors[0]->path);
    active->bindTo(&miniBuf);
    mode = 5;
    status = u"Switch to: ";
  } else {
    round++;
    if (round == cursors.size())
      round = 0;
    miniBuf = create(cursors[round]->path);
  }
}

void State::tryPaste() {
  const char *contents = glfwGetClipboardString(NULL);
  if (contents) {
    std::u16string str = create(std::string(contents));
    active->appendWithLines(str);
    if (mode != 0)
      return;
    if (hasHighlighting)
      highlighter.highlight(active->lines, &provider.colors, active->skip,
                            active->maxLines, active->y);
    status = u"Pasted " + numberToString(str.length()) + u" Characters";
  }
}

void State::cut() {
  if (!active->selection.active) {
    status = u"Aborted: No selection";
    return;
  }
  std::string content = active->getSelection();
  glfwSetClipboardString(NULL, content.c_str());
  active->deleteSelection();
  active->selection.stop();
  status = u"Cut " + numberToString(content.length()) + u" Characters";
}

void State::tryCopy() {
  if (!active->selection.active) {
    status = u"Aborted: No selection";
    return;
  }
  std::string content = active->getSelection();
  glfwSetClipboardString(NULL, content.c_str());
  active->selection.stop();
  status = u"Copied " + numberToString(content.length()) + u" Characters";
}

void State::save() {
  if (mode != 0)
    return;
  if (!path.length()) {
    saveNew();
    return;
  }
  active->saveTo(path);
  status = u"Saved: " + create(path);
}

void State::saveNew() {
  if (mode != 0)
    return;
  miniBuf = u"";
  active->bindTo(&miniBuf);
  mode = 1;
  status = u"Save to[" + create(provider.getCwdFormatted()) + u"]: ";
}

void State::changeFont() {
  if (mode != 0)
    return;
  miniBuf = create(provider.fontPath);
  active->bindTo(&miniBuf);
  mode = 15;
  status = u"Set font: ";
}

void State::open() {
  if (mode != 0)
    return;
  miniBuf = u"";
  provider.lastProvidedFolder = "";
  active->bindTo(&miniBuf);
  mode = 4;
  status = u"Open [" + create(provider.getCwdFormatted()) + u"]: ";
}

void State::reHighlight() {
  if (hasHighlighting)
    highlighter.highlight(active->lines, &provider.colors, active->skip,
                          active->maxLines, active->y);
}

void State::undo() {
  bool result = active->undo();
  status = result ? u"Undo" : u"Undo failed";
  if (result)
    reHighlight();
}

void State::search() {
  if (mode != 0)
    return;
  miniBuf = u"";
  active->bindTo(&miniBuf, true);
  mode = 2;
  status = u"Search: ";
}

void State::tryEnableHighlighting() {
  std::vector<std::u16string> fileParts = active->split(fileName, u".");
  std::string ext = convert_str(fileParts[fileParts.size() - 1]);
  const Language *lang =
      has_language(fileName == u"Dockerfile" ? "dockerfile" : ext);
  if (lang) {
    highlighter.setLanguage(*lang, lang->modeName);
    highlighter.highlight(active->lines, &provider.colors, active->skip,
                          active->maxLines, active->y);
    hasHighlighting = true;
  } else {
    hasHighlighting = false;
  }
}

void State::inform(bool success, bool shift_pressed) {
  if (success) {
    if (mode == 1) { // save to
      bool result = active->saveTo(convert_str(miniBuf));
      if (result) {
        status = u"Saved to: " + miniBuf;
        if (!path.length()) {
          path = convert_str(miniBuf);
          active->path = path;
          auto split = active->split(path, "/");
          std::string fName = split[split.size() - 1];
          fileName = create(fName);
          std::string window_name = "ledit: " + path;
          // glfwSetWindowTitle(window, window_name.c_str());
          tryEnableHighlighting();
        }
      } else {
        status = u"Failed to save to: " + miniBuf;
      }
    } else if (mode == 2 || mode == 7) { // search
      status = active->search(miniBuf, false, mode != 7);
      if (mode == 7)
        mode = 2;
      // hacky shit
      if (status != u"[Not found]: ")
        mode = 6;
      return;
    } else if (mode == 6) { // search
      status = active->search(miniBuf, true);
      if (status == u"[No further matches]: ") {
        mode = 7;
      }
      return;
    } else if (mode == 3) { // gotoline
      auto line_str = convert_str(miniBuf);
      if (isSafeNumber(line_str)) {
        active->gotoLine(std::stoi(line_str));
        status = u"Jump to: " + miniBuf;
      } else {
        status = u"Invalid line: " + miniBuf;
      }
    } else if (mode == 4 || mode == 5) {
      active->unbind();
      if (mode == 5) {
        if (round != getActiveIndex()) {
          activateCursor(round);
          status = u"Switched to: " + create(path.length() ? path : "New File");
        } else {
          status = u"Canceled";
        }
      } else {
        bool found = false;
        size_t fIndex = 0;
        auto converted = convert_str(miniBuf);
        for (size_t i = 0; i < cursors.size(); i++) {
          if (cursors[i]->path == converted) {
            found = true;
            fIndex = i;
            break;
          }
        }
        if (found && getActiveIndex() != fIndex)
          activateCursor(fIndex);
        else if (!found)
          addCursor(converted);
      }

    } else if (mode == 15) {
      atlas->readFont(convert_str(miniBuf), fontSize);
      provider.fontPath = convert_str(miniBuf);
      provider.writeConfig();
      status = u"Loaded font: " + miniBuf;
    } else if (mode == 25) {
      if (round == 0) {
        status = u"Mode: Text";
        hasHighlighting = false;
      } else {
        auto lang = getLanguage(round - 1);
        highlighter.setLanguage(lang, lang.modeName);
        hasHighlighting = true;
        status = u"Mode: " + miniBuf;
      }
    } else if (mode == 30) {
      replaceBuffer.search = miniBuf;
      miniBuf = replaceBuffer.replace;
      active->unbind();
      active->bindTo(&miniBuf);
      status = u"Replace: ";
      mode = 31;
      return;
    } else if (mode == 31) {
      mode = 32;
      replaceBuffer.replace = miniBuf;
      status = replaceBuffer.search + u" => " + replaceBuffer.replace;
      active->unbind();
      return;
    } else if (mode == 32) {
      if (shift_pressed) {
        auto count =
            active->replaceAll(replaceBuffer.search, replaceBuffer.replace);
        if (count)
          status = u"Replaced " + numberToString(count) + u" matches";
        else
          status = u"[No match]: " + replaceBuffer.search + u" => " +
                   replaceBuffer.replace;
      } else {
        auto result = active->replaceOne(replaceBuffer.search,
                                         replaceBuffer.replace, true);
        status =
            result + replaceBuffer.search + u" => " + replaceBuffer.replace;
        return;
      }
    } else if (mode == 36) {
      active->reloadFile(path);
      status = u"Reloaded";
    }
  } else {
    status = u"Aborted";
  }
  active->unbind();
  mode = 0;
}

void State::provideComplete(bool reverse) {
  if (mode == 4 || mode == 15 || mode == 1) {
    std::string convert = convert_str(miniBuf);
    std::string e = provider.getFileToOpen(
        convert == provider.getLast() ? provider.lastProvidedFolder : convert,
        reverse);
    if (!e.length())
      return;
    std::string p = provider.lastProvidedFolder;
    miniBuf = create(e);
  } else if (mode == 25) {
    if (reverse) {
      if (round == 0)
        round = getLanguageCount();
      else
        round--;
    } else {
      if (round == getLanguageCount())
        round = 0;
      else
        round++;
    }
    if (round == 0)
      miniBuf = u"Text";
    else
      miniBuf = create(getLanguage(round - 1).modeName);
  }
}

void State::renderCoords() {
  if (mode != 0)
    return;
  // if(hasHighlighting)
  // highlighter.highlight(active->lines, &provider.colors, active->skip,
  // active->maxLines, active->y);
  std::u16string branch;
  if (active->branch.length()) {
    branch = u" [git: " + create(active->branch) + u"]";
  }
  status = numberToString(active->y + 1) + u":" +
           numberToString(active->x + 1) + branch + u" [" + fileName + u": " +
           (hasHighlighting ? highlighter.languageName : u"Text") +
           u"] History Size: " + numberToString(active->history.size());
  if (active->selection.active)
    status +=
        u" Selected: [" + numberToString(active->getSelectionSize()) + u"]";
}

void State::gotoLine() {
  if (mode != 0)
    return;
  miniBuf = u"";
  active->bindTo(&miniBuf);
  mode = 3;
  status = u"Line: ";
}

std::u16string State::getTabInfo() {
  if (getActiveIndex() == 0 && cursors.size() == 1)
    return u"[ 1 ]";
  std::u16string text = u"[ " + numberToString(getActiveIndex() + 1) + u":" +
                        numberToString(cursors.size()) + u" ]";

  return text;
}

void State::rotateBuffer() {
  if (cursors.size() == 1)
    return;
  size_t next = getActiveIndex() + 1;
  if (next == cursors.size())
    next = 0;
  activateCursor(next);
}

void State::deleteCursor(const std::shared_ptr<Cursor> &cursor) {

  size_t i = 0;
  for (auto it = cursors.begin(); it != cursors.end(); ++it, ++i) {
    if (*it == cursor) {
      cursors.erase(it);
      if (cursor == active) {
        if (i + 1 < cursors.size()) {
          activateCursor(i + 1);
        } else {
          activateCursor(0);
        }
        return;
      }
    }
  }
}

void State::activateCursor(size_t cursorIndex) {
  active = cursors[cursorIndex];
  this->path = active->path;
  status = create(path);
  if (path.length()) {
    if (path == "-") {
      fileName = u"-(STDIN/OUT)";
      hasHighlighting = false;
      renderCoords();
    } else {
      auto split = active->split(path, "/");
      fileName = create(split[split.size() - 1]);
      tryEnableHighlighting();
      checkChanged();
    }
  } else {
    fileName = u"New File";
    hasHighlighting = false;
    renderCoords();
  }
  std::string window_name = "ledit: " + (path.length() ? path : "New File");
  // glfwSetWindowTitle(window, window_name.c_str());
}

void State::addCursor(std::string path) {
  if (path.length() && std::filesystem::is_directory(path)) {
    path = "";
  }

  auto newCursor = path.length() ? std::make_shared<Cursor>(path) : std::make_shared<Cursor>();
  if (path.length()) {
    newCursor->branch = provider.getBranchName(path);
  }
  cursors.push_back(newCursor);
  activateCursor(cursors.size() - 1);
}

void State::init() {
  atlas = std::make_shared<FontAtlas>(provider.fontPath, fontSize);
}
