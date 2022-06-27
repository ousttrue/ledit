#include "cursor.h"

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
