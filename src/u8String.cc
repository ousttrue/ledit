#include "u8String.h"

std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utfConverter;

std::string convert_str(std::u16string data) {
  std::string b = utfConverter.to_bytes(data);
  return b;
}

std::u16string create(std::string container) {
  try {
    std::u16string b = utfConverter.from_bytes(container);
    return b;
  } catch (std::exception e) {
    // failed to convert treat at ascii to display something at least
    std::u16string target = u"";
    std::string::const_iterator c;
    for (c = container.begin(); c != container.end(); c++) {
      target += (char16_t)*c;
    }
    return target;
  }
}

std::u16string numberToString(int value) {
  std::string val = std::to_string(value);
  return create(val);
}

