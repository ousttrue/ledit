#pragma once
#include <vector>
#include <string>
#include <filesystem>
// #include <iostream>
#include "la.h"
// #include "utils.h"
#include "../third-party/json/json.hpp"
// #ifndef _WIN32
// #include <signal.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #endif
// #ifdef __linux__
// #include <fontconfig/fontconfig.h>
// struct FontEntry {
//   std::string path;
//   std::string name;
//   std::string type;
// };
// #endif

// namespace fs = std::filesystem;
using json = nlohmann::json;

struct EditorColors {
  Vec4f string_color = vec4f(0.2, 0.6, 0.4, 1.0);
  Vec4f default_color = vec4fs(0.95);
  Vec4f keyword_color = vec4f(0.6, 0.1, 0.2, 1.0);
  Vec4f special_color = vec4f(0.2, 0.2, 0.8, 1.0);
  Vec4f number_color = vec4f(0.2, 0.2, 0.6, 1.0);
  Vec4f comment_color = vec4fs(0.5);
  Vec4f background_color = vec4f(0, 0, 0, 1.0);
  Vec4f highlight_color = vec4f(0.1, 0.1, 0.1, 1.0);
  Vec4f selection_color = vec4f(0.7, 0.7, 0.7, 0.6);
  Vec4f status_color = vec4f(0.8, 0.8, 1.0, 0.9);
  Vec4f minibuffer_color = vec4fs(1.0);
  Vec4f line_number_color = vec4fs(0.8);
};

class Provider {
public:
  std::string lastProvidedFolder;
  std::vector<std::string> folderEntries;
  int offset = 0;
  EditorColors colors;
  std::string fontPath = getDefaultFontPath();
  std::string configPath;
  bool allowTransparency = false;

  Provider();
  std::string getBranchName(std::string path);
  std::string getCwdFormatted();
  Vec4f getVecOrDefault(json o, const std::string entry, Vec4f def);
  bool getBoolOrDefault(json o, const std::string entry, bool def);
  std::string getPathOrDefault(json o, const std::string entry,
                               std::string def);
  const std::string getDefaultFontPath();
  const std::filesystem::path getDefaultFontDir();
  void parseConfig(json *configRoot);
  json vecToJson(Vec4f value);
  void writeConfig();
  std::string getLast();
  std::string getFileToOpen(std::string path, bool reverse);

private:
  std::filesystem::path *getHomeFolder();
};
