# Custom Brightness List Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the fixed brightness preset commands with a user-configurable brightness list edited through the TrafficMonitor plugin options dialog.

**Architecture:** Add a small configuration component that owns parsing, formatting, loading, and saving the brightness preset text. `Plugin` stores dynamic presets, builds command names from them, and maps command indices relative to the current preset count. `ShowOptionsDialog()` presents a minimal Win32 modal dialog with one edit control and saves the text through the configuration component.

**Tech Stack:** C++23, Win32 API, TrafficMonitor plugin interface, CMake/CTest, existing assert-based tests.

---

## File Structure

- Create `src/BrightnessPresetsConfig.h`: declares the preset parse result and config helper functions.
- Create `src/BrightnessPresetsConfig.cpp`: implements default text, parse/format logic, INI path construction, config file initialization, loading, and saving.
- Modify `src/Plugin.h`: add dynamic preset state, command-name storage, config path/text state, and `ShowOptionsDialog()` override.
- Modify `src/Plugin.cpp`: remove fixed brightness command array, load config on `EI_CONFIG_DIR`, generate dynamic command names, use preset values for brightness commands, and implement the modal options dialog.
- Modify `CMakeLists.txt`: compile `BrightnessPresetsConfig.cpp` into the plugin and `test_plugin` target.
- Modify `test/test_plugin.cpp`: add tests for parsing/config behavior and dynamic command mapping; update existing fixed-command assertions.

---

### Task 1: Add Brightness Preset Parsing and Formatting

**Files:**
- Create: `src/BrightnessPresetsConfig.h`
- Create: `src/BrightnessPresetsConfig.cpp`
- Modify: `CMakeLists.txt:9-14`, `CMakeLists.txt:38-42`
- Test: `test/test_plugin.cpp`

- [ ] **Step 1: Write failing parser tests**

Add this include near the top of `test/test_plugin.cpp`:

```cpp
#include "BrightnessPresetsConfig.h"
```

Add these test functions before `main()` in `test/test_plugin.cpp`:

```cpp
static void Test_BrightnessPresetParser_ParsesDefaults() {
  auto result = BrightnessPresetsConfig::ParsePresetText(L"0 25 50 75 100");

  assert(result.valid);
  assert((result.presets == std::vector<int>{0, 25, 50, 75, 100}));
}

static void Test_BrightnessPresetParser_DeduplicatesInInputOrder() {
  auto result = BrightnessPresetsConfig::ParsePresetText(L"50 20 50 0 20");

  assert(result.valid);
  assert((result.presets == std::vector<int>{50, 20, 0}));
}

static void Test_BrightnessPresetParser_InvalidTextClearsList() {
  auto invalidToken = BrightnessPresetsConfig::ParsePresetText(L"10 abc 50");
  auto outOfRange = BrightnessPresetsConfig::ParsePresetText(L"10 120 50");
  auto empty = BrightnessPresetsConfig::ParsePresetText(L"   ");

  assert(!invalidToken.valid);
  assert(invalidToken.presets.empty());
  assert(!outOfRange.valid);
  assert(outOfRange.presets.empty());
  assert(!empty.valid);
  assert(empty.presets.empty());
}

static void Test_BrightnessPresetFormatter_FormatsSpaceSeparatedText() {
  std::vector<int> presets{0, 25, 50, 75, 100};

  assert(BrightnessPresetsConfig::FormatPresetText(presets) == L"0 25 50 75 100");
}
```

Update `main()` in `test/test_plugin.cpp` so these tests run first:

```cpp
int main() {
  Test_BrightnessPresetParser_ParsesDefaults();
  Test_BrightnessPresetParser_DeduplicatesInInputOrder();
  Test_BrightnessPresetParser_InvalidTextClearsList();
  Test_BrightnessPresetFormatter_FormatsSpaceSeparatedText();
  Test_CommandNames_AreStable();
  Test_DataRequired_ReadsUntilFirstSuccessfulBrightness();
  Test_CommandBrightnessDelaysReadbackWithoutBlocking();
  Test_CommandBrightnessReadbackHandlesTickWraparound();
  Plugin::SetTickCountProviderForTest(nullptr);
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: FAIL at compile time because `BrightnessPresetsConfig.h` does not exist.

- [ ] **Step 3: Add config helper header**

Create `src/BrightnessPresetsConfig.h`:

```cpp
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
```

- [ ] **Step 4: Add minimal parser implementation**

Create `src/BrightnessPresetsConfig.cpp`:

```cpp
#include "BrightnessPresetsConfig.h"

#include <cwctype>
#include <sstream>
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

    int value = std::stoi(token);
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
```

- [ ] **Step 5: Wire config helper into CMake**

Modify the `DDCCIPlugin` library sources in `CMakeLists.txt`:

```cmake
add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
    src/MonitorController.cpp
)
```

Modify the `test_plugin` executable sources in `CMakeLists.txt`:

```cmake
add_executable(test_plugin
    test/test_plugin.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
)
```

- [ ] **Step 6: Run parser tests**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: PASS.

- [ ] **Step 7: Commit parser component**

```bash
git add CMakeLists.txt src/BrightnessPresetsConfig.h src/BrightnessPresetsConfig.cpp test/test_plugin.cpp
git commit -m "feat: add brightness preset parsing"
```

---

### Task 2: Add Config File Loading, Initialization, and Saving

**Files:**
- Modify: `src/BrightnessPresetsConfig.h`
- Modify: `src/BrightnessPresetsConfig.cpp`
- Test: `test/test_plugin.cpp`

- [ ] **Step 1: Write failing config file tests**

Add these includes near the top of `test/test_plugin.cpp`:

```cpp
#include <filesystem>
#include <fstream>
```

Add this helper before the parser tests in `test/test_plugin.cpp`:

```cpp
static std::filesystem::path TestConfigDir() {
  auto dir = std::filesystem::temp_directory_path() / L"DDCCIPluginTests";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}
```

Add these test functions before `main()`:

```cpp
static void Test_BrightnessPresetConfig_InitializesMissingFile() {
  auto dir = TestConfigDir();

  auto loaded = BrightnessPresetsConfig::LoadOrInitialize(dir.wstring());

  assert(loaded.text == L"0 25 50 75 100");
  assert(loaded.parse.valid);
  assert((loaded.parse.presets == std::vector<int>{0, 25, 50, 75, 100}));
  assert(std::filesystem::exists(dir / L"DDCCIPlugin.ini"));
}

static void Test_BrightnessPresetConfig_LoadsExistingFile() {
  auto dir = TestConfigDir();
  std::wofstream file(dir / L"DDCCIPlugin.ini");
  file << L"BrightnessPresets=10 30 70\n";
  file.close();

  auto loaded = BrightnessPresetsConfig::LoadOrInitialize(dir.wstring());

  assert(loaded.text == L"10 30 70");
  assert(loaded.parse.valid);
  assert((loaded.parse.presets == std::vector<int>{10, 30, 70}));
}

static void Test_BrightnessPresetConfig_SavesText() {
  auto dir = TestConfigDir();

  assert(BrightnessPresetsConfig::SavePresetText(dir.wstring(), L"5 55 95"));
  auto loaded = BrightnessPresetsConfig::LoadOrInitialize(dir.wstring());

  assert(loaded.text == L"5 55 95");
  assert((loaded.parse.presets == std::vector<int>{5, 55, 95}));
}
```

Update `main()` to run these after formatter tests:

```cpp
  Test_BrightnessPresetConfig_InitializesMissingFile();
  Test_BrightnessPresetConfig_LoadsExistingFile();
  Test_BrightnessPresetConfig_SavesText();
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: FAIL at compile time because `LoadOrInitialize()` and `SavePresetText()` are not declared.

- [ ] **Step 3: Declare config file helpers**

Update `src/BrightnessPresetsConfig.h`:

```cpp
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
```

- [ ] **Step 4: Implement config file helpers**

Add these includes to `src/BrightnessPresetsConfig.cpp`:

```cpp
#include <filesystem>
#include <fstream>
```

Add these helpers inside the anonymous namespace:

```cpp
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
```

Add these functions before the closing namespace in `src/BrightnessPresetsConfig.cpp`:

```cpp
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
```

- [ ] **Step 5: Run config file tests**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit config file support**

```bash
git add src/BrightnessPresetsConfig.h src/BrightnessPresetsConfig.cpp test/test_plugin.cpp
git commit -m "feat: persist brightness presets"
```

---

### Task 3: Replace Fixed Brightness Commands with Dynamic Presets

**Files:**
- Modify: `src/Plugin.h`
- Modify: `src/Plugin.cpp`
- Test: `test/test_plugin.cpp`

- [ ] **Step 1: Write failing dynamic command tests**

Modify the `MonitorController::SetBrightness` stub in `test/test_plugin.cpp`:

```cpp
static int g_lastSetBrightness = -1;

bool MonitorController::SetBrightness(int value) {
  ++g_setBrightnessCalls;
  g_lastSetBrightness = value;
  return true;
}
```

Update `ResetControllerState()`:

```cpp
static void ResetControllerState() {
  g_getBrightnessCalls = 0;
  g_setBrightnessCalls = 0;
  g_lastSetBrightness = -1;
  g_currentTick = 0;
}
```

Replace `Test_CommandNames_AreStable()` with:

```cpp
static void Test_CommandNames_UseDefaultDynamicPresets() {
  Plugin plugin;

  assert(plugin.GetCommandCount() == 7);
  assert(std::wcscmp(plugin.GetCommandName(-1), L"") == 0);
  assert(std::wcscmp(plugin.GetCommandName(0), L"亮度 0%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"亮度 25%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(2), L"亮度 50%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(3), L"亮度 75%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(4), L"亮度 100%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(5), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(6), L"电源 关机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(7), L"") == 0);
}

static void Test_CommandBrightness_UsesPresetValue() {
  ResetControllerState();
  Plugin::SetTickCountProviderForTest(TestGetTickCount);
  Plugin plugin;

  plugin.OnPluginCommand(1, nullptr, nullptr);

  assert(g_setBrightnessCalls == 1);
  assert(g_lastSetBrightness == 25);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"25%") == 0);
}
```

Update `main()` to call the renamed and new tests:

```cpp
  Test_CommandNames_UseDefaultDynamicPresets();
  Test_CommandBrightness_UsesPresetValue();
```

Remove the old `Test_CommandNames_AreStable();` call.

- [ ] **Step 2: Run test to verify it fails**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: FAIL because the plugin still exposes 13 fixed commands and command index 1 sets brightness 10.

- [ ] **Step 3: Add dynamic preset members to Plugin**

Update `src/Plugin.h`:

```cpp
#pragma once

#include "BrightnessItem.h"
#include "PluginInterface.h"
#include <string>
#include <vector>
#include <windows.h>

class Plugin : public ITMPlugin {
public:
  using TickCountProvider = DWORD (*)();

  int GetAPIVersion() const override;
  IPluginItem *GetItem(int index) override;
  void DataRequired() override;
  const wchar_t *GetInfo(PluginInfoIndex index) override;
  int GetCommandCount() override;
  const wchar_t *GetCommandName(int command_index) override;
  void OnPluginCommand(int command_index, void *hWnd, void *para) override;
  void OnInitialize(ITrafficMonitor *pApp) override;
  void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;

  static void SetTickCountProviderForTest(TickCountProvider provider);

private:
  ITrafficMonitor *m_pApp = nullptr;
  BrightnessItem m_brightnessItem;

  bool m_mainNoPercent = false;
  bool m_mainSpaceBeforeUnit = false;
  bool m_taskbarNoPercent = false;
  bool m_taskbarSpaceBeforeUnit = false;
  bool m_drawTaskbarWnd = false;
  bool m_hasMainNoPercent = false;
  bool m_hasMainSpaceBeforeUnit = false;
  bool m_hasTaskbarNoPercent = false;
  bool m_hasTaskbarSpaceBeforeUnit = false;
  bool m_brightnessNeedsUpdate = true;
  DWORD m_nextBrightnessUpdateTick = 0;
  std::vector<int> m_brightnessPresets;
  std::vector<std::wstring> m_brightnessCommandNames;

  void ApplyDisplayOptions();
  void ApplyPresetText(const std::wstring &text);
};
```

- [ ] **Step 4: Replace fixed command constants and command logic**

In `src/Plugin.cpp`, replace the fixed `kCommandNames`, brightness count, power index constants, and static assert with:

```cpp
static constexpr const wchar_t *kStandbyCommandName = L"电源 待机";
static constexpr const wchar_t *kTurnOffCommandName = L"电源 关机";
```

Add this include:

```cpp
#include "BrightnessPresetsConfig.h"
#include <string>
```

Add a constructor-equivalent initialization by defining this helper method before `GetAPIVersion()`:

```cpp
void Plugin::ApplyPresetText(const std::wstring &text) {
  auto parsed = BrightnessPresetsConfig::ParsePresetText(text);
  m_brightnessPresets = parsed.presets;
  m_brightnessCommandNames.clear();

  for (int preset : m_brightnessPresets) {
    m_brightnessCommandNames.push_back(L"亮度 " + std::to_wstring(preset) + L"%");
  }
}
```

Add a real constructor declaration in `src/Plugin.h` public section:

```cpp
  Plugin();
```

Add the constructor implementation before `GetAPIVersion()` in `src/Plugin.cpp`:

```cpp
Plugin::Plugin() { ApplyPresetText(BrightnessPresetsConfig::DefaultPresetText()); }
```

Replace `GetCommandCount()`:

```cpp
int Plugin::GetCommandCount() {
  return static_cast<int>(m_brightnessPresets.size()) + 2;
}
```

Replace `GetCommandName()`:

```cpp
const wchar_t *Plugin::GetCommandName(int command_index) {
  if (command_index < 0 || command_index >= GetCommandCount()) {
    return L"";
  }

  int brightnessCount = static_cast<int>(m_brightnessPresets.size());
  if (command_index < brightnessCount) {
    return m_brightnessCommandNames[command_index].c_str();
  }

  if (command_index == brightnessCount) {
    return kStandbyCommandName;
  }

  return kTurnOffCommandName;
}
```

Replace the command branch in `OnPluginCommand()`:

```cpp
  int brightnessCount = static_cast<int>(m_brightnessPresets.size());
  if (command_index >= 0 && command_index < brightnessCount) {
    int value = m_brightnessPresets[command_index];
    if (MonitorController::SetBrightness(value)) {
      m_brightnessItem.UpdateBrightness(value);
      m_brightnessNeedsUpdate = true;
      m_nextBrightnessUpdateTick =
          CurrentTickCount() + kBrightnessReadbackDelayMs;
    }
  } else if (command_index == brightnessCount) {
    MonitorController::Standby();
  } else if (command_index == brightnessCount + 1) {
    MonitorController::TurnOff();
  }
```

- [ ] **Step 5: Run dynamic command tests**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit dynamic command support**

```bash
git add src/Plugin.h src/Plugin.cpp test/test_plugin.cpp
git commit -m "feat: use dynamic brightness commands"
```

---

### Task 4: Load Presets from TrafficMonitor Config Directory

**Files:**
- Modify: `src/Plugin.h`
- Modify: `src/Plugin.cpp`
- Test: `test/test_plugin.cpp`

- [ ] **Step 1: Write failing plugin config load tests**

Add these tests before `main()`:

```cpp
static void Test_Plugin_LoadsPresetsFromConfigDirectory() {
  auto dir = TestConfigDir();
  BrightnessPresetsConfig::SavePresetText(dir.wstring(), L"5 55 95");
  Plugin plugin;

  plugin.OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.wstring().c_str());

  assert(plugin.GetCommandCount() == 5);
  assert(std::wcscmp(plugin.GetCommandName(0), L"亮度 5%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"亮度 55%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(2), L"亮度 95%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(3), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(4), L"电源 关机") == 0);
}

static void Test_Plugin_InvalidConfigShowsOnlyPowerCommands() {
  auto dir = TestConfigDir();
  BrightnessPresetsConfig::SavePresetText(dir.wstring(), L"10 abc 50");
  Plugin plugin;

  plugin.OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.wstring().c_str());

  assert(plugin.GetCommandCount() == 2);
  assert(std::wcscmp(plugin.GetCommandName(0), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"电源 关机") == 0);
}
```

Update `main()` to run these after dynamic command tests:

```cpp
  Test_Plugin_LoadsPresetsFromConfigDirectory();
  Test_Plugin_InvalidConfigShowsOnlyPowerCommands();
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: FAIL because `Plugin::OnExtenedInfo(EI_CONFIG_DIR, ...)` does not load preset config yet.

- [ ] **Step 3: Store config directory and raw config text**

Add members to `src/Plugin.h` private section:

```cpp
  std::wstring m_configDir;
  std::wstring m_presetText;
```

Update `Plugin::ApplyPresetText()` in `src/Plugin.cpp`:

```cpp
void Plugin::ApplyPresetText(const std::wstring &text) {
  m_presetText = text;
  auto parsed = BrightnessPresetsConfig::ParsePresetText(text);
  m_brightnessPresets = parsed.presets;
  m_brightnessCommandNames.clear();

  for (int preset : m_brightnessPresets) {
    m_brightnessCommandNames.push_back(L"亮度 " + std::to_wstring(preset) + L"%");
  }
}
```

- [ ] **Step 4: Handle EI_CONFIG_DIR in OnExtenedInfo**

Add this case to the `switch` in `Plugin::OnExtenedInfo()` before `default`:

```cpp
  case EI_CONFIG_DIR:
    if (data != nullptr) {
      m_configDir = data;
      auto config = BrightnessPresetsConfig::LoadOrInitialize(m_configDir);
      ApplyPresetText(config.text);
    }
    break;
```

- [ ] **Step 5: Run config load tests**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit config loading**

```bash
git add src/Plugin.h src/Plugin.cpp test/test_plugin.cpp
git commit -m "feat: load brightness presets from config"
```

---

### Task 5: Add Options Dialog Save Path

**Files:**
- Modify: `src/Plugin.h`
- Modify: `src/Plugin.cpp`
- Test: `test/test_plugin.cpp`

- [ ] **Step 1: Write failing save helper tests**

Add this public test-only helper declaration to `src/Plugin.h` public section:

```cpp
  OptionReturn SavePresetTextForTest(const std::wstring &text);
```

Add these tests before `main()`:

```cpp
static void Test_Plugin_SavePresetTextUpdatesCommandsAndFile() {
  auto dir = TestConfigDir();
  Plugin plugin;
  plugin.OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.wstring().c_str());

  auto result = plugin.SavePresetTextForTest(L"15 45 90");

  assert(result == ITMPlugin::OR_OPTION_CHANGED);
  assert(plugin.GetCommandCount() == 5);
  assert(std::wcscmp(plugin.GetCommandName(0), L"亮度 15%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"亮度 45%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(2), L"亮度 90%") == 0);
  auto loaded = BrightnessPresetsConfig::LoadOrInitialize(dir.wstring());
  assert(loaded.text == L"15 45 90");
}

static void Test_Plugin_SaveInvalidPresetTextKeepsOnlyPowerCommands() {
  auto dir = TestConfigDir();
  Plugin plugin;
  plugin.OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.wstring().c_str());

  auto result = plugin.SavePresetTextForTest(L"15 bad 90");

  assert(result == ITMPlugin::OR_OPTION_CHANGED);
  assert(plugin.GetCommandCount() == 2);
  assert(std::wcscmp(plugin.GetCommandName(0), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"电源 关机") == 0);
  auto loaded = BrightnessPresetsConfig::LoadOrInitialize(dir.wstring());
  assert(loaded.text == L"15 bad 90");
}
```

Update `main()` to run these after config load tests:

```cpp
  Test_Plugin_SavePresetTextUpdatesCommandsAndFile();
  Test_Plugin_SaveInvalidPresetTextKeepsOnlyPowerCommands();
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: FAIL because `SavePresetTextForTest()` has no implementation.

- [ ] **Step 3: Implement the save helper**

Add this implementation to `src/Plugin.cpp` before `OnInitialize()`:

```cpp
ITMPlugin::OptionReturn Plugin::SavePresetTextForTest(const std::wstring &text) {
  if (m_configDir.empty()) {
    return OR_OPTION_UNCHANGED;
  }

  if (!BrightnessPresetsConfig::SavePresetText(m_configDir, text)) {
    return OR_OPTION_UNCHANGED;
  }

  ApplyPresetText(text);
  return OR_OPTION_CHANGED;
}
```

- [ ] **Step 4: Add ShowOptionsDialog override**

Add this declaration to `src/Plugin.h` public section next to other ITMPlugin overrides:

```cpp
  OptionReturn ShowOptionsDialog(void *hParent) override;
```

Add these includes to `src/Plugin.cpp`:

```cpp
#include <commctrl.h>
```

Add this implementation before `OnInitialize()` in `src/Plugin.cpp`:

```cpp
ITMPlugin::OptionReturn Plugin::ShowOptionsDialog(void *hParent) {
  constexpr int kBufferSize = 256;
  wchar_t buffer[kBufferSize] = {};
  wcsncpy_s(buffer, m_presetText.c_str(), _TRUNCATE);

  HWND parent = static_cast<HWND>(hParent);
  HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, L"STATIC", L"显示器控制设置",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 360, 150, parent,
                                nullptr, GetModuleHandleW(nullptr), nullptr);
  if (dialog == nullptr) {
    return OR_OPTION_UNCHANGED;
  }

  CreateWindowExW(0, L"STATIC", L"亮度预设（空格分隔，0 到 100）：",
                  WS_CHILD | WS_VISIBLE, 12, 14, 320, 20, dialog, nullptr,
                  GetModuleHandleW(nullptr), nullptr);
  HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buffer,
                              WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 12, 40,
                              320, 24, dialog, reinterpret_cast<HMENU>(1001),
                              GetModuleHandleW(nullptr), nullptr);
  HWND ok = CreateWindowExW(0, L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE,
                            176, 80, 72, 24, dialog,
                            reinterpret_cast<HMENU>(IDOK), GetModuleHandleW(nullptr),
                            nullptr);
  HWND cancel = CreateWindowExW(0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE,
                                260, 80, 72, 24, dialog,
                                reinterpret_cast<HMENU>(IDCANCEL),
                                GetModuleHandleW(nullptr), nullptr);
  (void)ok;
  (void)cancel;

  EnableWindow(parent, FALSE);
  ShowWindow(dialog, SW_SHOW);
  SetFocus(edit);

  OptionReturn result = OR_OPTION_UNCHANGED;
  MSG msg;
  bool done = false;
  while (!done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (msg.hwnd == dialog || IsChild(dialog, msg.hwnd)) {
      if (msg.message == WM_COMMAND) {
        int command = LOWORD(msg.wParam);
        if (command == IDOK) {
          GetWindowTextW(edit, buffer, kBufferSize);
          result = SavePresetTextForTest(buffer);
          done = true;
          continue;
        }
        if (command == IDCANCEL) {
          done = true;
          continue;
        }
      }
      if (msg.message == WM_CLOSE) {
        done = true;
        continue;
      }
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  EnableWindow(parent, TRUE);
  DestroyWindow(dialog);
  return result;
}
```

- [ ] **Step 5: Run save helper tests**

Run:

```bash
cmake --build build --target test_plugin && ctest --test-dir build -R test_plugin --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit options save path**

```bash
git add src/Plugin.h src/Plugin.cpp test/test_plugin.cpp
git commit -m "feat: add brightness preset options dialog"
```

---

### Task 6: Final Verification

**Files:**
- Verify: `CMakeLists.txt`
- Verify: `src/BrightnessPresetsConfig.h`
- Verify: `src/BrightnessPresetsConfig.cpp`
- Verify: `src/Plugin.h`
- Verify: `src/Plugin.cpp`
- Verify: `test/test_plugin.cpp`

- [ ] **Step 1: Run the full test suite**

Run:

```bash
cmake --build build && ctest --test-dir build --output-on-failure
```

Expected: all configured tests PASS, including `test_brightness_item` and `test_plugin`.

- [ ] **Step 2: Check git status**

Run:

```bash
git status --short
```

Expected: no uncommitted changes if each task was committed.

- [ ] **Step 3: Manual plugin check**

Build the plugin DLL:

```bash
cmake --build build --target DDCCIPlugin
```

Expected: build succeeds and produces the plugin DLL under the existing build output directory.

Install or load the plugin in TrafficMonitor, open the plugin options, enter:

```text
0 25 50 75 100
```

Expected: the plugin command menu shows `亮度 0%`, `亮度 25%`, `亮度 50%`, `亮度 75%`, `亮度 100%`, then `电源 待机`, `电源 关机`.

Then enter:

```text
15 bad 90
```

Expected: the plugin command menu shows only `电源 待机` and `电源 关机`.

---

## Self-Review

- Spec coverage: the plan covers the options dialog, config file under `EI_CONFIG_DIR`, default initialization to `0 25 50 75 100`, space-separated parsing, invalid-list behavior, duplicate removal with input order, dynamic command names, dynamic command execution, and tests.
- Placeholder scan: no `TBD`, `TODO`, omitted test details, or open-ended implementation steps remain.
- Type consistency: `BrightnessPresetsConfig::ParseResult`, `LoadedConfig`, `Plugin::ApplyPresetText`, and `Plugin::SavePresetTextForTest` are introduced before later steps use them.
