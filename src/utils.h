#pragma once
#include <string>
#include <vector>

extern const std::u16string wordSeperator;

std::string file_to_string(std::string path);
bool hasEnding(std::u16string fullString, std::u16string ending);
bool hasEnding(std::string fullString, std::string ending);
bool isSafeNumber(std::string value);
bool string_to_file(std::string path, std::string content);

static std::vector<std::string> split(std::string base, std::string delimiter) {
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

static std::vector<std::u16string> split(std::u16string base,
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
