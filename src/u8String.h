#pragma once
#include <string>
#include <codecvt>
#ifdef __linux__
#include <locale>
#endif
typedef union char_s {
  char16_t value;
  uint8_t arr[2];
} char_t;

std::string convert_str(std::u16string data);
std::u16string create(std::string container);
std::u16string numberToString(int value);
