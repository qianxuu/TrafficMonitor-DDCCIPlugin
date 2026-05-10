#pragma once

#include <string>
#include <vector>

namespace BrightnessPresetsConfig {

struct ParseResult {
  bool valid = false;
  std::vector<int> presets;
};

struct LoadedConfig {
  std::wstring text;
  ParseResult parse;
};

const std::wstring &DefaultPresetText();
ParseResult ParsePresetText(const std::wstring &text);
std::wstring FormatPresetText(const std::vector<int> &presets);
LoadedConfig LoadOrInitialize(const std::wstring &configDir);
bool SavePresetText(const std::wstring &configDir, const std::wstring &text);

} // namespace BrightnessPresetsConfig
