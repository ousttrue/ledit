#pragma once
#include <string>

extern const std::u16string wordSeperator;

std::string file_to_string(std::string path);
bool hasEnding(std::u16string fullString, std::u16string ending);
bool hasEnding(std::string fullString, std::string ending);
bool isSafeNumber(std::string value);
bool string_to_file(std::string path, std::string content);
