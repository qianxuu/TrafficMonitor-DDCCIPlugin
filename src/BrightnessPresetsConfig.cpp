#include "BrightnessPresetsConfig.h"

#include <cwctype>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace BrightnessPresetsConfig {
namespace {
const std::wstring kDefaultPresetText = L"0 25 50 75 100";

bool IsDigitsOnly(const std::wstring &token) {
  if (token.empty()) {
    return false;
  }

  for (wchar_t ch : token) {
    if (!std::iswdigit(ch)) {
      return false;
    }
  }

  return true;
}
} // namespace

const std::wstring &DefaultPresetText() { return kDefaultPresetText; }

ParseResult ParsePresetText(const std::wstring &text) {
  ParseResult result;
  std::wistringstream stream(text);
  std::wstring token;
  std::unordered_set<int> seen;

  while (stream >> token) {
    if (!IsDigitsOnly(token)) {
      return {};
    }

    int value = 0;
    try {
      value = std::stoi(token);
    } catch (const std::out_of_range &) {
      return {};
    }
    if (value < 0 || value > 100) {
      return {};
    }

    if (seen.insert(value).second) {
      result.presets.push_back(value);
    }
  }

  result.valid = !result.presets.empty();
  if (!result.valid) {
    result.presets.clear();
  }

  return result;
}

std::wstring FormatPresetText(const std::vector<int> &presets) {
  std::wostringstream stream;

  for (size_t i = 0; i < presets.size(); ++i) {
    if (i > 0) {
      stream << L' ';
    }
    stream << presets[i];
  }

  return stream.str();
}

} // namespace BrightnessPresetsConfig
