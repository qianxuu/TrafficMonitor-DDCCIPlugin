#pragma once

#include <string>
#include <vector>

namespace BrightnessPresetsConfig {

struct ParseResult {
  bool valid = false;
  std::vector<int> presets;
};

const std::wstring &DefaultPresetText();
ParseResult ParsePresetText(const std::wstring &text);
std::wstring FormatPresetText(const std::vector<int> &presets);

} // namespace BrightnessPresetsConfig
