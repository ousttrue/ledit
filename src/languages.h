#pragma once
#include "highlighting.h"
const Language* has_language(std::string ext);
size_t getLanguageCount();
const Language& getLanguage(size_t index);