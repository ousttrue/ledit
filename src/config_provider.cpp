#include "config_provider.h"
#include "utils.h"
#include <iostream>

Provider::Provider() {
  std::filesystem::path *homeDir = getHomeFolder();
  if (homeDir) {
    std::filesystem::path configDir = *homeDir / ".ledit";
    if (std::filesystem::exists(configDir)) {
      std::filesystem::path configFile = configDir / "config.json";
      configPath = configFile.generic_string();
      if (std::filesystem::exists(configFile)) {
        std::string contents = file_to_string(configPath);
        json parsed = json::parse(contents);
        parseConfig(&parsed);
      } else {
        json j;
        parseConfig(&j);
      }
    } else {
      std::filesystem::create_directory(configDir);
      json j;
      parseConfig(&j);
    }
    delete homeDir;
  } else {
    std::cerr << "Failed to load home env var\n";
  }
}

std::string Provider::getBranchName(std::string path) {
  std::string asPath =
      std::filesystem::path(path).parent_path().generic_string();
  const char *as_cstr = asPath.c_str();
  std::string branch = "";
#ifdef _WIN32
  std::string command = "cd " + asPath + " && git branch";
  FILE *pipe = _popen(command.c_str(), "r");
  if (pipe == NULL)
    return branch;
  char buffer[1024];
  while (fgets(buffer, sizeof buffer, pipe) != NULL) {
    branch += buffer;
  }
  _pclose(pipe);
#else
  int fd[2], pid;
  pipe(fd);
  pid = fork();
  if (pid == 0) {
    close(1);
    dup(fd[1]);
    close(0);
    close(2);
    close(fd[0]);
    close(fd[1]);
    const char *args[] = {"git", "branch", nullptr};
    chdir(as_cstr);
    execvp("git", static_cast<char *const *>((void *)args));
    exit(errno);
  } else {
    close(fd[1]);
    while (true) {
      char buffer[1024];
      int received = read(fd[0], buffer, 1024);
      if (received == 0)
        break;
      branch += std::string(buffer, received);
    }
    close(fd[0]);
    kill(pid, SIGTERM);
    wait(&pid);
  }
#endif
  if (!branch.length())
    return branch;
  std::string finalBranch = branch.substr(branch.find("* ") + 2);
  return finalBranch.substr(0, finalBranch.find("\n"));
}

std::string Provider::getCwdFormatted() {
  std::string path =
#ifdef _WIN32
      std::filesystem::current_path().generic_string();
#else
      std::filesystem::current_path();
#endif
  std::filesystem::path *homeDir = getHomeFolder();
  bool isHomeDirectory =
      homeDir != nullptr && path.find((*homeDir).generic_string()) == 0;
  std::string target = path;
  if (isHomeDirectory) {
    target = target.substr((*homeDir).generic_string().length());
    target = "~" + target;
  }
  if (homeDir != nullptr)
    delete homeDir;
  return target;
}

Vec4f Provider::getVecOrDefault(json o, const std::string entry, Vec4f def) {
  if (!o.contains(entry))
    return def;
  json e = o[entry];
  if (!e.is_array() || e.size() != 4)
    return def;
  for (auto &element : e) {
    int val = element;
    if (val < 0 || val > 255)
      return def;
  }
  return vec4f(((float)e[0] / (float)255), ((float)e[1] / (float)255),
               ((float)e[2] / (float)255), ((float)e[3] / (float)255));
}

bool Provider::getBoolOrDefault(json o, const std::string entry, bool def) {
  if (!o.contains(entry))
    return def;
  json e = o[entry];
  if (!e.is_boolean())
    return def;

  return (bool)e;
}

std::string Provider::getPathOrDefault(json o, const std::string entry,
                                       std::string def) {
  if (!o.contains(entry))
    return def;
  std::string e = o[entry];
  if (!std::filesystem::exists(e))
    return def;
  return e;
}

const std::string Provider::getDefaultFontPath() {
#ifdef _WIN32
  return (getDefaultFontDir() / "consola.ttf").generic_string();
#endif
#ifdef __APPLE__
  return (getDefaultFontDir() / "Monaco.ttf").generic_string();
#endif
#ifdef __linux__
  FcConfig *config = FcInitLoadConfigAndFonts();
  FcPattern *pat = FcPatternCreate();
  FcObjectSet *objectSet = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG,
                                            FC_FILE, FC_SPACING, nullptr);
  FcFontSet *fSet = FcFontList(config, pat, objectSet);
  std::vector<FontEntry> results;
  for (int i = 0; fSet && i < fSet->nfont; i++) {
    FcPattern *font = fSet->fonts[i];
    FcChar8 *file, *family, *style;
    int spacing;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
        FcPatternGetInteger(font, FC_SPACING, 0, &spacing) == FcResultMatch &&
        FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
        FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) {
      if (spacing == 100)
        results.push_back({std::string((const char *)file),
                           std::string((const char *)family),
                           std::string((const char *)style)});
    }
  }
  if (fSet)
    FcFontSetDestroy(fSet);
  for (auto &entry : results) {
    if (entry.name == "Hack" && entry.type == "Regular")
      return entry.path;
  }
  for (auto &entry : results) {
    if (entry.type == "Regular")
      return entry.path;
  }
  return results[0].path;
#endif
}

const std::filesystem::path Provider::getDefaultFontDir() {
// no idea how to do this differently
#ifdef _WIN32
  return "C:\\Windows\\Fonts";
#endif
#ifdef __APPLE__
  return "/System/Library/Fonts";
#else
  // fontconfig used
  return "";
#endif
}

void Provider::parseConfig(json *configRoot) {
  if (configRoot->contains("colors")) {
    json configColors = (*configRoot)["colors"];
    colors.string_color =
        getVecOrDefault(configColors, "string_color", colors.string_color);
    colors.default_color =
        getVecOrDefault(configColors, "default_color", colors.default_color);
    colors.keyword_color =
        getVecOrDefault(configColors, "keyword_color", colors.keyword_color);
    colors.special_color =
        getVecOrDefault(configColors, "special_color", colors.special_color);
    colors.comment_color =
        getVecOrDefault(configColors, "comment_color", colors.comment_color);
    colors.background_color = getVecOrDefault(configColors, "background_color",
                                              colors.background_color);
    colors.highlight_color = getVecOrDefault(configColors, "highlight_color",
                                             colors.highlight_color);
    colors.selection_color = getVecOrDefault(configColors, "selection_color",
                                             colors.selection_color);
    colors.number_color =
        getVecOrDefault(configColors, "number_color", colors.number_color);
    colors.status_color =
        getVecOrDefault(configColors, "status_color", colors.status_color);
    colors.line_number_color = getVecOrDefault(
        configColors, "line_number_color", colors.line_number_color);
    colors.minibuffer_color = getVecOrDefault(configColors, "minibuffer_color",
                                              colors.minibuffer_color);
  }
  fontPath = getPathOrDefault(*configRoot, "font_face", fontPath);
  allowTransparency =
      getBoolOrDefault(*configRoot, "window_transparency", allowTransparency);
}

json Provider::vecToJson(Vec4f value) {
  json j;
  j.push_back((int)(255 * value.x));
  j.push_back((int)(255 * value.y));
  j.push_back((int)(255 * value.z));
  j.push_back((int)(255 * value.w));
  return j;
}

void Provider::writeConfig() {
  if (!configPath.length())
    return;
  json config;
  json cColors;
  cColors["string_color"] = vecToJson(colors.string_color);
  cColors["default_color"] = vecToJson(colors.default_color);
  cColors["keyword_color"] = vecToJson(colors.keyword_color);
  cColors["special_color"] = vecToJson(colors.special_color);
  cColors["comment_color"] = vecToJson(colors.comment_color);
  cColors["background_color"] = vecToJson(colors.background_color);
  cColors["highlight_color"] = vecToJson(colors.highlight_color);
  cColors["selection_color"] = vecToJson(colors.selection_color);
  cColors["number_color"] = vecToJson(colors.number_color);
  cColors["status_color"] = vecToJson(colors.status_color);
  cColors["line_number_color"] = vecToJson(colors.line_number_color);
  cColors["minibuffer_color"] = vecToJson(colors.minibuffer_color);
  config["font_face"] = fontPath;
  config["window_transparency"] = allowTransparency;
  config["colors"] = cColors;
  const std::string contents = config.dump(2);
  string_to_file(configPath, contents);
}

std::string Provider::getLast() {
  if (!folderEntries.size())
    return "";
  return folderEntries[offset];
}

std::string Provider::getFileToOpen(std::string path, bool reverse) {
  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
    return "";
  if (lastProvidedFolder == path) {
    offset += reverse ? -1 : 1;

    if (offset == folderEntries.size())
      offset = 0;
    else if (offset == -1)
      offset = folderEntries.size() - 1;
    return folderEntries[offset];
  }
  folderEntries.clear();
  for (auto const &dir_entry : std::filesystem::directory_iterator{path}) {
    folderEntries.push_back(dir_entry.path().string());
  }
  offset = 0;
  lastProvidedFolder = path;
  return folderEntries[offset];
}

std::filesystem::path *Provider::getHomeFolder() {
#ifdef _WIN32
  const char *home = getenv("USERPROFILE");
#else
  const char *home = getenv("HOME");
#endif
  if (home)
    return new std::filesystem::path(home);
  return nullptr;
}
