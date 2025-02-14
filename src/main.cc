#if (MAC_OS_X_VERSION_MAX_ALLOWED < 120000) // Before macOS 12 Monterey
#define kIOMainPortDefault kIOMasterPortDefault
#endif

#include "state.h"
#include "font_atlas.h"
#include "glfwapp.h"
#include "renderer.h"
#include "glutil/gpu.h"
#include "glutil/drawable.h"
#include "glutil/shader.h"
#include "glutil/texture.h"
#include <memory>
#include <iostream>
#include <fstream>
#ifndef __APPLE__
#include <algorithm>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif

VertexLayout textVertexLayout[] = {
    {2, offsetof(RenderChar, pos), 1},
    {2, offsetof(RenderChar, size), 1},
    {2, offsetof(RenderChar, uv_pos), 1},
    {2, offsetof(RenderChar, uv_size), 1},
    {4, offsetof(RenderChar, fg_color), 1},
    {4, offsetof(RenderChar, bg_color), 1},
};

struct SelectionEntry {
  Vec2f pos;
  Vec2f size;
};

VertexLayout selVertexLayout[] = {
    {2, offsetof(SelectionEntry, pos), 1},
    {2, offsetof(SelectionEntry, size), 1},
};

int main(int argc, char **argv) {
#ifdef _WIN32
  ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
  std::string initialPath = argc >= 2 ? std::string(argv[1]) : "";

  const std::string window_name =
      "ledit: " + (initialPath.length() ? initialPath : "New File");
  GlfwApp app;

  State state(1280, 720);
  auto getProc =
      app.createWindow(window_name.c_str(), state.WIDTH, state.HEIGHT, &state,
                       state.provider.allowTransparency);
  if (!getProc) {
    return 1;
  }

  if (!gpu::initialize(getProc)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return 2;
  }

  state.addCursor(initialPath);
  // state.window = window;

  auto fontHeight = 30;
  auto fontWidth = 15;
  auto atlas = std::make_shared<FontAtlas>(state.provider.fontPath, fontHeight);

  auto text = std::shared_ptr<Drawable>(new Drawable(
      Shader::createText(), sizeof(RenderChar), textVertexLayout,
      _countof(textVertexLayout), sizeof(RenderChar) * 600 * 1000));

  auto selection = std::shared_ptr<Drawable>(new Drawable(
      Shader::createSelection(), sizeof(SelectionEntry), selVertexLayout,
      _countof(selVertexLayout), sizeof(SelectionEntry) * 16));

  auto cursor_shader = Shader::createCursor();

  // float xscale, yscale;
  // std::tie(xscale, yscale) = app.getScale();
  // std::cout << state.WIDTH << ", " << state.HEIGHT << ":" << xscale << ", "
  //           << yscale << std::endl;
  // state.WIDTH *= xscale;
  // state.HEIGHT *= yscale;

  int fontSize = 0;
  float WIDTH = 0;
  float HEIGHT = 0;
  Renderer r;
  auto maxRenderWidth = 0;
  while (app.isWindowAlive()) {
    if (state.cacheValid) {
      app.wait();
      continue;
    }

    if (HEIGHT != state.HEIGHT || WIDTH != state.WIDTH) {
      WIDTH = state.WIDTH;
      state.highlighter.wasCached = false;
      HEIGHT = state.HEIGHT;
    }

    auto cursor = state.active;
    float toOffset = atlas->getHeight() * 1.15;
    bool isSearchMode = state.mode == 2 || state.mode == 6 || state.mode == 7 ||
                        state.mode == 32;
    cursor->setBounds(HEIGHT - atlas->getHeight() - 6, toOffset);
    if (maxRenderWidth != 0) {
      cursor->getContent(fontWidth, maxRenderWidth, true);
    }

    auto be_color = state.provider.colors.background_color;
    auto status_color = state.provider.colors.status_color;

    gpu::clear(state.WIDTH, state.HEIGHT, &be_color.x);

    if (state.highlightLine) {
      selection->use();
      auto color = state.provider.colors.highlight_color;
      selection->set("selection_color", color.x, color.y, color.z, color.w);
      selection->set("resolution", WIDTH, HEIGHT);
      SelectionEntry entry{vec2f((-(int32_t)WIDTH / 2) + 10,
                                 (float)HEIGHT / 2 - 5 - toOffset -
                                     ((cursor->_y - cursor->_skip) * toOffset)),
                           vec2f((((int32_t)WIDTH / 2) * 2) - 20, toOffset)};
      selection->drawUploadInstance(&entry, sizeof(SelectionEntry), 6, 1);
    }

    text->use();
    text->set("resolution", WIDTH, HEIGHT);

    atlas->getTexture()->bind(0);

    // glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    std::u16string::const_iterator c;
    std::string::const_iterator cc;
    float xpos = (-(int32_t)WIDTH / 2) + 10;
    float ypos = -(float)HEIGHT / 2;
    int start = cursor->_skip;
    int maxLines = cursor->_skip + (cursor->_maxLines <= cursor->_lines.size()
                                        ? cursor->_skip + cursor->_maxLines
                                        : cursor->_lines.size());
    // if (state.showLineNumbers) {
    //   int biggestLine = std::to_string(maxLines).length();
    //   auto maxLineAdvance = atlas->getAdvance(std::to_string(maxLines));
    //   for (int i = start; i < maxLines; i++) {
    //     std::string value = std::to_string(i + 1);
    //     auto tAdvance = atlas->getAdvance(value);
    //     xpos += maxLineAdvance - tAdvance;
    //     linesAdvance = 0;
    //     for (cc = value.begin(); cc != value.end(); cc++) {
    //       entries.push_back(atlas->render(
    //           *cc, xpos, ypos, state.provider.colors.line_number_color));
    //       auto advance = atlas->getAdvance(*cc);
    //       xpos += advance;
    //       linesAdvance += advance;
    //     }
    //     xpos = -(int32_t)WIDTH / 2 + 10;
    //     ypos += toOffset;
    //   }
    // }

    auto &entries =
        r.render(WIDTH, HEIGHT, cursor, atlas, fontWidth, {1, 1, 1, 1});
    text->drawUploadInstance(&entries[0], sizeof(RenderChar) * entries.size(),
                             6, entries.size());

    if (state.focused) {
      // cursor
      cursor_shader->use();
      cursor_shader->set1f("cursor_height", toOffset);
      cursor_shader->set2f("resolution", (float)WIDTH, (float)HEIGHT);
      if (state.mode != 0 && state.mode != 32) {
        // use cursor for minibuffer
        float cursorX = -(int32_t)(WIDTH / 2) + 15 +
                        (atlas->getAdvance(cursor->getCurrentAdvance())) + 5 +
                        atlas->getAdvance(state.status);
        float cursorY = (float)HEIGHT / 2 - 10;
        cursor_shader->set2f("cursor_pos", cursorX, -cursorY);
        text->drawTriangleStrip(4);
      }

      //   if (isSearchMode || state.mode == 0) {
      //     float cursorX =
      //         -(int32_t)(WIDTH / 2) + 15 +
      //         (atlas->getAdvance(cursor->getCurrentAdvance(isSearchMode))) +
      //         linesAdvance + 4 - cursor->_xSkip;
      //     if (cursorX > WIDTH / 2)
      //       cursorX = (WIDTH / 2) - 3;
      //     float cursorY = -(int32_t)(HEIGHT / 2) + 4 +
      //                     (toOffset * ((cursor->_y - cursor->_skip) + 1));
      //     cursor_shader->set2f("cursor_pos", cursorX, -cursorY);
      //     text->drawTriangleStrip(4);
      //   }
      // }
      // if (cursor->_selection.active) {
      //   std::vector<SelectionEntry> selectionBoundaries;
      //   if (cursor->_selection.getYSmaller() < cursor->_skip &&
      //       cursor->_selection.getYBigger() > cursor->_skip +
      //       cursor->_maxLines) {
      //     // select everything
      //   } else {
      //     maxRenderWidth += atlas->getAdvance(u" ");
      //     int yStart = cursor->_selection.getYStart();
      //     int yEnd = cursor->_selection.getYEnd();
      //     if (cursor->_selection.yStart == cursor->_selection.yEnd) {
      //       if (cursor->_selection.xStart != cursor->_selection.xEnd) {
      //         int smallerX = cursor->_selection.getXSmaller();
      //         if (smallerX >= cursor->_xOffset) {

      //           float renderDistance = atlas->getAdvance(
      //               (*allLines)[yEnd - cursor->_skip].second.substr(
      //                   0, smallerX - cursor->_xOffset));
      //           float renderDistanceBigger = atlas->getAdvance(
      //               (*allLines)[yEnd - cursor->_skip].second.substr(
      //                   0, cursor->_selection.getXBigger() -
      //                   cursor->_xOffset));
      //           if (renderDistance < maxRenderWidth * 2) {
      //             float start = ((float)HEIGHT / 2) - 5 -
      //                           (toOffset * ((yEnd - cursor->_skip) + 1));
      //             selectionBoundaries.push_back(
      //                 {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
      //                            renderDistance,
      //                        start),
      //                  vec2f(renderDistanceBigger - renderDistance,
      //                  toOffset)});
      //           } else {
      //             float renderDistanceBigger = atlas->getAdvance(
      //                 (*allLines)[yEnd - cursor->_skip].second.substr(
      //                     0, cursor->_selection.getXBigger() -
      //                     cursor->_xOffset));
      //             float start = ((float)HEIGHT / 2) - 5 -
      //                           (toOffset * ((yEnd - cursor->_skip) + 1));
      //             selectionBoundaries.push_back(
      //                 {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
      //                            (maxRenderWidth - renderDistance),
      //                        start),
      //                  vec2f(maxRenderWidth > renderDistanceBigger
      //                            ? maxRenderWidth
      //                            : renderDistanceBigger,
      //                        toOffset)});
      //           }
      //         } else {
      //           float renderDistanceBigger = atlas->getAdvance(
      //               (*allLines)[yEnd - cursor->_skip].second.substr(
      //                   0, cursor->_selection.getXBigger() -
      //                   cursor->_xOffset));
      //           float start = ((float)HEIGHT / 2) - 5 -
      //                         (toOffset * ((yEnd - cursor->_skip) + 1));
      //           selectionBoundaries.push_back(
      //               {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance, start),
      //                vec2f(renderDistanceBigger > maxRenderWidth * 2
      //                          ? maxRenderWidth * 2
      //                          : renderDistanceBigger,
      //                      toOffset)});
      //         }
      //       }
      //     } else {
      //       if (yStart >= cursor->_skip &&
      //           yStart <= cursor->_skip + cursor->_maxLines) {
      //         int yEffective = cursor->_selection.getYStart() -
      //         cursor->_skip; int xStart = cursor->_selection.getXStart(); if
      //         (xStart >= cursor->_xOffset) {
      //           float renderDistance =
      //               atlas->getAdvance((*allLines)[yEffective].second.substr(
      //                   0, xStart - cursor->_xOffset));
      //           if (renderDistance < maxRenderWidth) {
      //             if (yStart < yEnd) {

      //               float start =
      //                   ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective +
      //                   1));
      //               selectionBoundaries.push_back(
      //                   {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
      //                              renderDistance,
      //                          start),
      //                    vec2f((maxRenderWidth * 2) - renderDistance,
      //                    toOffset)});
      //             } else {
      //               float start =
      //                   ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective +
      //                   1));
      //               selectionBoundaries.push_back(
      //                   {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance,
      //                   start),
      //                    vec2f(renderDistance, toOffset)});
      //             }
      //           }
      //         }
      //       }
      //       if (yEnd >= cursor->_skip &&
      //           yEnd <= cursor->_skip + cursor->_maxLines) {
      //         int yEffective = cursor->_selection.getYEnd() - cursor->_skip;
      //         int xStart = cursor->_selection.getXEnd();
      //         if (xStart >= cursor->_xOffset) {
      //           float renderDistance =
      //               atlas->getAdvance((*allLines)[yEffective].second.substr(
      //                   0, xStart - cursor->_xOffset));
      //           if (renderDistance < maxRenderWidth) {
      //             if (yEnd < yStart) {
      //               float start =
      //                   ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective +
      //                   1));
      //               selectionBoundaries.push_back(
      //                   {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance +
      //                              renderDistance,
      //                          start),
      //                    vec2f((maxRenderWidth * 2) - renderDistance,
      //                    toOffset)});
      //             } else {
      //               float start =
      //                   ((float)HEIGHT / 2) - 5 - (toOffset * (yEffective +
      //                   1));
      //               selectionBoundaries.push_back(
      //                   {vec2f(-(int32_t)WIDTH / 2 + 20 + linesAdvance,
      //                   start),
      //                    vec2f(renderDistance, toOffset)});
      //             }
      //           }
      //         }
      //       }
      //       bool found = false;
      //       int offset = 0;
      //       int count = 0;
      //       for (int i = cursor->_selection.getYSmaller();
      //            i < cursor->_selection.getYBigger() - 1; i++) {
      //         if (i > cursor->_skip + cursor->_maxLines)
      //           break;
      //         if (i >= cursor->_skip - 1) {
      //           if (!found) {
      //             found = true;
      //             offset = i - cursor->_skip;
      //           }
      //           count++;
      //         }
      //       }
      //       if (found) {
      //         float start = (float)HEIGHT / 2 - 5 - (toOffset * (offset +
      //         1)); selectionBoundaries.push_back(
      //             {vec2f((-(int32_t)WIDTH / 2) + 20 + linesAdvance, start),
      //              vec2f(maxRenderWidth * 2, -(count * toOffset))});
      //       }
      //     }
      //   }
      //   if (selectionBoundaries.size()) {
      //     selection->use();
      //     auto color = state.provider.colors.selection_color;
      //     selection->set("selection_color", color.x, color.y, color.z,
      //     color.w); selection->set("resolution", WIDTH, HEIGHT);

      //     selection->drawUploadInstance(&selectionBoundaries[0],
      //                                   sizeof(SelectionEntry) *
      //                                       selectionBoundaries.size(),
      //                                   6, selectionBoundaries.size());
      //   }
    }

    app.flush();
    state.cacheValid = true;
  }

  return 0;
};
