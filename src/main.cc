#if (MAC_OS_X_VERSION_MAX_ALLOWED < 120000) // Before macOS 12 Monterey
#define kIOMainPortDefault kIOMasterPortDefault
#endif

#include "glfwapp.h"
#include "shader.h"
#ifndef __APPLE__
#include <algorithm>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif
#include "state.h"
#include "cursor.h"
#include "shaders.h"
#include "font_atlas.h"
#include "vertex_buffer.h"

int main(int argc, char **argv) {
#ifdef _WIN32
  ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
  std::string initialPath = argc >= 2 ? std::string(argv[1]) : "";

  const std::string window_name =
      "ledit: " + (initialPath.length() ? initialPath : "New File");
  GlfwApp app;

  State state(1280, 720, 30);
  if (!app.createWindow(window_name.c_str(), state.WIDTH, state.HEIGHT, &state,
                        state.provider.allowTransparency)) {
    return -1;
  }

  state.addCursor(initialPath);
  // state.window = window;

  // OpenGL state
  // ------------
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  state.init();
  Shader text_shader(text_shader_vert, text_shader_frag, {});
  text_shader.use();
  Shader cursor_shader(cursor_shader_vert, cursor_shader_frag,
                       {camera_shader_vert});
  Shader selection_shader(selection_shader_vert, selection_shader_frag, {});
  float xscale, yscale;
  std::tie(xscale, yscale) = app.getScale();
  state.WIDTH *= xscale;
  state.HEIGHT *= yscale;
  int fontSize = 0;
  float WIDTH = 0;
  float HEIGHT = 0;
  auto maxRenderWidth = 0;
  while (app.isWindowAlive()) {
    if (state.cacheValid) {
      app.wait();
      continue;
    }

    bool changed = false;
    if (HEIGHT != state.HEIGHT || WIDTH != state.WIDTH ||
        fontSize != state.fontSize) {
      WIDTH = state.WIDTH;
      fontSize = state.fontSize;
      state.highlighter.wasCached = false;
      HEIGHT = state.HEIGHT;
      changed = true;
    }

    Cursor *cursor = state.cursor;
    float toOffset = state.atlas->atlas_height * 1.15;
    bool isSearchMode = state.mode == 2 || state.mode == 6 || state.mode == 7 ||
                        state.mode == 32;
    cursor->setBounds(HEIGHT - state.atlas->atlas_height - 6, toOffset);
    if (maxRenderWidth != 0) {
      cursor->getContent(state.atlas.get(), maxRenderWidth, true);
    }

    auto be_color = state.provider.colors.background_color;
    auto status_color = state.provider.colors.status_color;
    glClearColor(be_color.x, be_color.y, be_color.z, be_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state.highlightLine) {
      selection_shader.use();
      glBindVertexArray(state.highlight_vao);
      auto color = state.provider.colors.highlight_color;
      selection_shader.set4f("selection_color", color.x, color.y, color.z,
                             color.w);
      selection_shader.set2f("resolution", (float)WIDTH, (float)HEIGHT);
      glBindBuffer(GL_ARRAY_BUFFER, state.highlight_vbo);
      SelectionEntry entry{vec2f((-(int32_t)WIDTH / 2) + 10,
                                 (float)HEIGHT / 2 - 5 - toOffset -
                                     ((cursor->y - cursor->skip) * toOffset)),
                           vec2f((((int32_t)WIDTH / 2) * 2) - 20, toOffset)};
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SelectionEntry), &entry);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 1);
    }
    text_shader.use();
    text_shader.set2f("resolution", (float)WIDTH, (float)HEIGHT);
    glActiveTexture(GL_TEXTURE0);
    state.vao->bind();
    glBindTexture(GL_TEXTURE_2D, state.atlas->texture_id);
    // glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    std::vector<RenderChar> entries;
    std::u16string::const_iterator c;
    std::string::const_iterator cc;
    float xpos = (-(int32_t)WIDTH / 2) + 10;
    float ypos = -(float)HEIGHT / 2;
    int start = cursor->skip;
    float linesAdvance = 0;
    int maxLines = cursor->skip + cursor->maxLines <= cursor->lines.size()
                       ? cursor->skip + cursor->maxLines
                       : cursor->lines.size();
    if (state.showLineNumbers) {
      int biggestLine = std::to_string(maxLines).length();
      auto maxLineAdvance = state.atlas->getAdvance(std::to_string(maxLines));
      for (int i = start; i < maxLines; i++) {
        std::string value = std::to_string(i + 1);
        auto tAdvance = state.atlas->getAdvance(value);
        xpos += maxLineAdvance - tAdvance;
        linesAdvance = 0;
        for (cc = value.begin(); cc != value.end(); cc++) {
          entries.push_back(state.atlas->render(
              *cc, xpos, ypos, state.provider.colors.line_number_color));
          auto advance = state.atlas->getAdvance(*cc);
          xpos += advance;
          linesAdvance += advance;
        }
        xpos = -(int32_t)WIDTH / 2 + 10;
        ypos += toOffset;
      }
    }
    maxRenderWidth = (WIDTH / 2) - 20 - linesAdvance;
    auto skipNow = cursor->skip;
    auto *allLines =
        cursor->getContent(state.atlas.get(), maxRenderWidth, false);
    state.reHighlight();
    ypos = (-(HEIGHT / 2));
    xpos = -(int32_t)WIDTH / 2 + 20 + linesAdvance;
    cursor->setRenderStart(20 + linesAdvance, 15);
    Vec4f color = state.provider.colors.default_color;
    if (state.hasHighlighting) {
      auto highlighter = state.highlighter;
      int lineOffset = cursor->skip;
      auto *colored = state.highlighter.get();
      int cOffset = cursor->getTotalOffset();
      int cxOffset = cursor->xOffset;
      //        std::cout << cxOffset << ":" << lineOffset << "\n";

      for (size_t x = 0; x < allLines->size(); x++) {
        auto content = (*allLines)[x].second;
        auto hasColorIndex = highlighter.lineIndex.count(x + lineOffset);
        if (content.length())
          cOffset += cxOffset;
        else
          cOffset += (*allLines)[x].first;
        if (cxOffset > 0) {
          if (hasColorIndex) {
            auto entry = highlighter.lineIndex[x + lineOffset];
            auto start = colored->begin();
            std::advance(start, entry.first);
            auto end = colored->begin();
            std::advance(end, entry.second);
            for (std::map<int, Vec4f>::iterator it = start; it != end; ++it) {
              int xx = it->first;
              if (xx >= cOffset)
                break;
              color = it->second;
            }
          }
        }
        if ((*colored).count(cOffset)) {
          color = (*colored)[cOffset];
        }
        int charAdvance = 0;
        for (c = content.begin(); c != content.end(); c++) {
          if ((*colored).count(cOffset)) {
            color = (*colored)[cOffset];
          }

          cOffset++;
          charAdvance++;
          if (*c != '\t')
            entries.push_back(state.atlas->render(*c, xpos, ypos, color));
          xpos += state.atlas->getAdvance(*c);
          if (xpos > (maxRenderWidth + state.atlas->getAdvance(*c)) &&
              c != content.end()) {
            int remaining = content.length() - (charAdvance);

            if (remaining > 0) {
              if (hasColorIndex) {
                auto entry = highlighter.lineIndex[x + lineOffset];
                auto start = colored->begin();
                std::advance(start, entry.first);
                auto end = colored->begin();
                std::advance(end, entry.second);
                for (std::map<int, Vec4f>::iterator it = start; it != end;
                     ++it) {
                  int xx = it->first;
                  if (xx > cOffset + remaining)
                    break;
                  if (xx >= cOffset)
                    color = it->second;
                }
              }
              cOffset += remaining;
            }

            break;
          }
        }

        if (x < allLines->size() - 1) {
          if ((*colored).count(cOffset)) {
            color = (*colored)[cOffset];
          }
          cOffset++;
          xpos = -maxRenderWidth;
          ypos += toOffset;
        }
      }
    } else {
      for (size_t x = 0; x < allLines->size(); x++) {
        auto content = (*allLines)[x].second;
        for (c = content.begin(); c != content.end(); c++) {
          if (*c != '\t')
            entries.push_back(state.atlas->render(*c, xpos, ypos, color));
          xpos += state.atlas->getAdvance(*c);
          if (xpos > maxRenderWidth + state.atlas->getAdvance(*c)) {
            break;
          }
        }
        if (x < allLines->size() - 1) {
          xpos = -maxRenderWidth;
          ypos += toOffset;
        }
      }
    }
    xpos = (-(int32_t)WIDTH / 2) + 15;
    ypos = (float)HEIGHT / 2 - toOffset - 10;
    std::u16string status = state.status;
    for (c = status.begin(); c != status.end(); c++) {
      entries.push_back(state.atlas->render(*c, xpos, ypos, status_color));
      xpos += state.atlas->getAdvance(*c);
    }
    float statusAdvance = state.atlas->getAdvance(state.status);
    if (state.mode != 0 && state.mode != 32) {
      // draw minibuffer
      xpos = (-(int32_t)WIDTH / 2) + 20 + statusAdvance;
      ypos = (float)HEIGHT / 2 - toOffset - 10;
      std::u16string status = state.miniBuf;
      for (c = status.begin(); c != status.end(); c++) {
        entries.push_back(state.atlas->render(
            *c, xpos, ypos, state.provider.colors.minibuffer_color));
        xpos += state.atlas->getAdvance(*c);
      }

    } else {
      auto tabInfo = state.getTabInfo();
      xpos = ((int32_t)WIDTH / 2) - state.atlas->getAdvance(tabInfo);
      ypos = (float)HEIGHT / 2 - toOffset - 10;
      for (c = tabInfo.begin(); c != tabInfo.end(); c++) {
        entries.push_back(state.atlas->render(*c, xpos, ypos, status_color));
        xpos += state.atlas->getAdvance(*c);
      }
    }

    state.vbo->upload(&entries[0], sizeof(RenderChar) * entries.size());

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)entries.size());
    if (state.focused) {
      cursor_shader.use();
      cursor_shader.set1f("cursor_height", toOffset);
      cursor_shader.set2f("resolution", (float)WIDTH, (float)HEIGHT);
      if (state.mode != 0 && state.mode != 32) {
        // use cursor for minibuffer
        float cursorX = -(int32_t)(WIDTH / 2) + 15 +
                        (state.atlas->getAdvance(cursor->getCurrentAdvance())) +
                        5 + statusAdvance;
        float cursorY = (float)HEIGHT / 2 - 10;
        cursor_shader.set2f("cursor_pos", cursorX, -cursorY);
        state.vao->drawTriangleStrip(4);
        glBindTexture(GL_TEXTURE_2D, 0);
      }

      if (isSearchMode || state.mode == 0) {
        float cursorX =
            -(int32_t)(WIDTH / 2) + 15 +
            (state.atlas->getAdvance(cursor->getCurrentAdvance(isSearchMode))) +
            linesAdvance + 4 - cursor->xSkip;
        if (cursorX > WIDTH / 2)
          cursorX = (WIDTH / 2) - 3;
        float cursorY = -(int32_t)(HEIGHT / 2) + 4 +
                        (toOffset * ((cursor->y - cursor->skip) + 1));
        cursor_shader.set2f("cursor_pos", cursorX, -cursorY);
        state.vao->drawTriangleStrip(4);
        glBindTexture(GL_TEXTURE_2D, 0);
      }
    }
    if (cursor->selection.active) {
      std::vector<SelectionEntry> selectionBoundaries;
      if (cursor->selection.getYSmaller() < cursor->skip &&
          cursor->selection.getYBigger() > cursor->skip + cursor->maxLines) {
        // select everything
      } else {
        maxRenderWidth += state.atlas->getAdvance(u" ");
        int yStart = cursor->selection.getYStart();
        int yEnd = cursor->selection.getYEnd();
        if (cursor->selection.yStart == cursor->selection.yEnd) {
          if (cursor->selection.xStart != cursor->selection.xEnd) {
            int smallerX = cursor->selection.getXSmaller();
            if (smallerX >= cursor->xOffset) {

              float renderDistance = state.atlas->getAdvance(
                  (*allLines)[yEnd - cursor->skip].second.substr(
                      0, smallerX - cursor->xOffset));
              float renderDistanceBigger = state.atlas->getAdvance(
                  (*allLines)[yEnd - cursor->skip].second.substr(
                      0, cursor->selection.getXBigger() - cursor->xOffset));
              if (renderDistance < maxRenderWidth * 2) {
                float start = ((float)HEIGHT / 2) - 5 -
                              (toOffset * ((yEnd - cursor->skip) + 1));
                selectionBoundaries.push_back(
                    {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
                               renderDistance,
                           start),
                     vec2f(renderDistanceBigger - renderDistance, toOffset)});
              } else {
                float renderDistanceBigger = state.atlas->getAdvance(
                    (*allLines)[yEnd - cursor->skip].second.substr(
                        0, cursor->selection.getXBigger() - cursor->xOffset));
                float start = ((float)HEIGHT / 2) - 5 -
                              (toOffset * ((yEnd - cursor->skip) + 1));
                selectionBoundaries.push_back(
                    {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
                               (maxRenderWidth - renderDistance),
                           start),
                     vec2f(maxRenderWidth > renderDistanceBigger
                               ? maxRenderWidth
                               : renderDistanceBigger,
                           toOffset)});
              }
            } else {
              float renderDistanceBigger = state.atlas->getAdvance(
                  (*allLines)[yEnd - cursor->skip].second.substr(
                      0, cursor->selection.getXBigger() - cursor->xOffset));
              float start = ((float)HEIGHT / 2) - 5 -
                            (toOffset * ((yEnd - cursor->skip) + 1));
              selectionBoundaries.push_back(
                  {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance, start),
                   vec2f(renderDistanceBigger > maxRenderWidth * 2
                             ? maxRenderWidth * 2
                             : renderDistanceBigger,
                         toOffset)});
            }
          }
        } else {
          if (yStart >= cursor->skip &&
              yStart <= cursor->skip + cursor->maxLines) {
            int yEffective = cursor->selection.getYStart() - cursor->skip;
            int xStart = cursor->selection.getXStart();
            if (xStart >= cursor->xOffset) {
              float renderDistance =
                  state.atlas->getAdvance((*allLines)[yEffective].second.substr(
                      0, xStart - cursor->xOffset));
              if (renderDistance < maxRenderWidth) {
                if (yStart < yEnd) {

                  float start =
                      ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective + 1));
                  selectionBoundaries.push_back(
                      {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
                                 renderDistance,
                             start),
                       vec2f((maxRenderWidth * 2) - renderDistance, toOffset)});
                } else {
                  float start =
                      ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective + 1));
                  selectionBoundaries.push_back(
                      {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance, start),
                       vec2f(renderDistance, toOffset)});
                }
              }
            }
          }
          if (yEnd >= cursor->skip && yEnd <= cursor->skip + cursor->maxLines) {
            int yEffective = cursor->selection.getYEnd() - cursor->skip;
            int xStart = cursor->selection.getXEnd();
            if (xStart >= cursor->xOffset) {
              float renderDistance =
                  state.atlas->getAdvance((*allLines)[yEffective].second.substr(
                      0, xStart - cursor->xOffset));
              if (renderDistance < maxRenderWidth) {
                if (yEnd < yStart) {
                  float start =
                      ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective + 1));
                  selectionBoundaries.push_back(
                      {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
                                 renderDistance,
                             start),
                       vec2f((maxRenderWidth * 2) - renderDistance, toOffset)});
                } else {
                  float start =
                      ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective + 1));
                  selectionBoundaries.push_back(
                      {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance, start),
                       vec2f(renderDistance, toOffset)});
                }
              }
            }
          }
          bool found = false;
          int offset = 0;
          int count = 0;
          for (int i = cursor->selection.getYSmaller();
               i < cursor->selection.getYBigger() - 1; i++) {
            if (i > cursor->skip + cursor->maxLines)
              break;
            if (i >= cursor->skip - 1) {
              if (!found) {
                found = true;
                offset = i - cursor->skip;
              }
              count++;
            }
          }
          if (found) {
            float start = (float)HEIGHT / 2 - 5 - (toOffset * (offset + 1));
            selectionBoundaries.push_back(
                {vec2f((-(int32_t)WIDTH / 2) + 20 + linesAdvance, start),
                 vec2f(maxRenderWidth * 2, -(count * toOffset))});
          }
        }
      }
      if (selectionBoundaries.size()) {
        selection_shader.use();
        glBindVertexArray(state.sel_vao);
        auto color = state.provider.colors.selection_color;
        selection_shader.set4f("selection_color", color.x, color.y, color.z,
                               color.w);
        selection_shader.set2f("resolution", (float)WIDTH, (float)HEIGHT);
        glBindBuffer(GL_ARRAY_BUFFER, state.sel_vbo);

        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        sizeof(SelectionEntry) * selectionBoundaries.size(),
                        &selectionBoundaries[0]);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6,
                              (GLsizei)selectionBoundaries.size());
      }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    app.flush();
    state.cacheValid = true;
  }
  return 0;
};
