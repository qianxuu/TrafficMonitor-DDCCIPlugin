#include "BrightnessPresetsConfig.h"

#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace BrightnessPresetsConfig {
namespace {
const std::wstring kDefaultPresetText = L"0 25 50 75 100";
const wchar_t *kConfigFileName = L"DDCCIPlugin.ini";
const wchar_t *kConfigKey = L"BrightnessPresets=";

std::filesystem::path ConfigPath(const std::wstring &configDir) {
  return std::filesystem::path(configDir) / kConfigFileName;
}

std::wstring ReadPresetText(const std::filesystem::path &path) {
  std::wifstream file(path);
  std::wstring line;

  while (std::getline(file, line)) {
    if (line.starts_with(kConfigKey)) {
      return line.substr(std::wcslen(kConfigKey));
    }
  }

  return L"";
}

bool WritePresetText(const std::filesystem::path &path, const std::wstring &text) {
  std::wofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }

  file << kConfigKey << text << L'\n';
  return static_cast<bool>(file);
}

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

LoadedConfig LoadOrInitialize(const std::wstring &configDir) {
  auto path = ConfigPath(configDir);
  std::error_code error;

  if (!std::filesystem::exists(path, error)) {
    if (!WritePresetText(path, DefaultPresetText())) {
      return {L"", {}};
    }
  }

  auto text = ReadPresetText(path);
  return {text, ParsePresetText(text)};
}

bool SavePresetText(const std::wstring &configDir, const std::wstring &text) {
  return WritePresetText(ConfigPath(configDir), text);
}

} // namespace BrightnessPresetsConfig
