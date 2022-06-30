#include "renderer.h"
#include "document.h"
#include "font_atlas.h"

std::vector<RenderChar> &
Renderer::render(int WIDTH, int HEIGHT, const std::shared_ptr<Document> &cursor,
                 const std::shared_ptr<FontAtlas> &atlas, int fontWidth,
                 const Vec4f &color) {
  entries.clear();
  float linesAdvance = 0;
  auto maxRenderWidth = (WIDTH / 2) - 20 - linesAdvance;
  auto skipNow = cursor->_skip;
  auto *allLines = cursor->getContent(fontWidth, maxRenderWidth, false);
  // state.reHighlight();
  auto ypos = (-(HEIGHT / 2));
  auto xpos = -(int32_t)WIDTH / 2 + 20 + linesAdvance;
  cursor->setRenderStart(20 + linesAdvance, 15);
  float toOffset = atlas->getHeight() * 1.15;
  // Vec4f color = state.provider.colors.default_color;
  // if (state.hasHighlighting) {
  //   auto highlighter = state.highlighter;
  //   int lineOffset = cursor->_skip;
  //   auto *colored = state.highlighter.get();
  //   int cOffset = cursor->getTotalOffset();
  //   int cxOffset = cursor->_xOffset;
  //   //        std::cout << cxOffset << ":" << lineOffset << "\n";

  //   for (size_t x = 0; x < allLines->size(); x++) {
  //     auto content = (*allLines)[x].second;
  //     auto hasColorIndex = highlighter.lineIndex.count(x + lineOffset);
  //     if (content.length())
  //       cOffset += cxOffset;
  //     else
  //       cOffset += (*allLines)[x].first;
  //     if (cxOffset > 0) {
  //       if (hasColorIndex) {
  //         auto entry = highlighter.lineIndex[x + lineOffset];
  //         auto start = colored->begin();
  //         std::advance(start, entry.first);
  //         auto end = colored->begin();
  //         std::advance(end, entry.second);
  //         for (std::map<int, Vec4f>::iterator it = start; it != end; ++it) {
  //           int xx = it->first;
  //           if (xx >= cOffset)
  //             break;
  //           color = it->second;
  //         }
  //       }
  //     }
  //     if ((*colored).count(cOffset)) {
  //       color = (*colored)[cOffset];
  //     }
  //     int charAdvance = 0;
  //     for (c = content.begin(); c != content.end(); c++) {
  //       if ((*colored).count(cOffset)) {
  //         color = (*colored)[cOffset];
  //       }

  //       cOffset++;
  //       charAdvance++;
  //       if (*c != '\t')
  //         entries.push_back(r.atlas->render(*c, xpos, ypos, color));
  //       xpos += r.atlas->getAdvance(*c);
  //       if (xpos > (maxRenderWidth + r.atlas->getAdvance(*c)) &&
  //           c != content.end()) {
  //         int remaining = content.length() - (charAdvance);

  //         if (remaining > 0) {
  //           if (hasColorIndex) {
  //             auto entry = highlighter.lineIndex[x + lineOffset];
  //             auto start = colored->begin();
  //             std::advance(start, entry.first);
  //             auto end = colored->begin();
  //             std::advance(end, entry.second);
  //             for (std::map<int, Vec4f>::iterator it = start; it != end;
  //                  ++it) {
  //               int xx = it->first;
  //               if (xx > cOffset + remaining)
  //                 break;
  //               if (xx >= cOffset)
  //                 color = it->second;
  //             }
  //           }
  //           cOffset += remaining;
  //         }

  //         break;
  //       }
  //     }

  //     if (x < allLines->size() - 1) {
  //       if ((*colored).count(cOffset)) {
  //         color = (*colored)[cOffset];
  //       }
  //       cOffset++;
  //       xpos = -maxRenderWidth;
  //       ypos += toOffset;
  //     }
  //   }
  // } else
  {
    for (size_t x = 0; x < allLines->size(); x++) {
      auto content = (*allLines)[x].second;
      for (auto c = content.begin(); c != content.end(); c++) {
        if (*c != '\t')
          entries.push_back(atlas->render(*c, xpos, ypos, color));
        xpos += atlas->getAdvance(*c);
        if (xpos > maxRenderWidth + atlas->getAdvance(*c)) {
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
  // std::u16string status = state.status;
  // for (c = status.begin(); c != status.end(); c++) {
  //   entries.push_back(r.atlas->render(*c, xpos, ypos, status_color));
  //   xpos += r.atlas->getAdvance(*c);
  // }
  // float statusAdvance = r.atlas->getAdvance(state.status);
  // if (state.mode != 0 && state.mode != 32) {
  //   // draw minibuffer
  //   xpos = (-(int32_t)WIDTH / 2) + 20 + statusAdvance;
  //   ypos = (float)HEIGHT / 2 - toOffset - 10;
  //   std::u16string status = state.miniBuf;
  //   for (c = status.begin(); c != status.end(); c++) {
  //     entries.push_back(r.atlas->render(
  //         *c, xpos, ypos, state.provider.colors.minibuffer_color));
  //     xpos += r.atlas->getAdvance(*c);
  //   }

  // }
  // else {
  //   auto tabInfo = state.getTabInfo();
  //   xpos = ((int32_t)WIDTH / 2) - r.atlas->getAdvance(tabInfo);
  //   ypos = (float)HEIGHT / 2 - toOffset - 10;
  //   for (c = tabInfo.begin(); c != tabInfo.end(); c++) {
  //     entries.push_back(r.atlas->render(*c, xpos, ypos, status_color));
  //     xpos += r.atlas->getAdvance(*c);
  //   }
  // }

  return entries;
}
