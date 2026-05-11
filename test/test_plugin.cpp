#include "BrightnessPresetsConfig.h"
#include "MonitorController.h"
#include "Plugin.h"
#include <cassert>
#include <cwchar>
#include <filesystem>
#include <fstream>

static int g_getBrightnessCalls = 0;
static int g_setBrightnessCalls = 0;
static int g_lastSetBrightness = -1;
static DWORD g_currentTick = 0;
static int g_brightnessValues[] = {-1, 42, 80};

static void ResetControllerState() {
  g_getBrightnessCalls = 0;
  g_setBrightnessCalls = 0;
  g_lastSetBrightness = -1;
  g_currentTick = 0;
}

DWORD TestGetTickCount() { return g_currentTick; }

bool MonitorController::Standby() { return true; }

bool MonitorController::TurnOff() { return true; }

bool MonitorController::SetBrightness(int value) {
  ++g_setBrightnessCalls;
  g_lastSetBrightness = value;
  return true;
}

int MonitorController::GetBrightness() {
  int index = g_getBrightnessCalls;
  ++g_getBrightnessCalls;
  return g_brightnessValues[index];
}

static std::filesystem::path TestConfigDir() {
  auto dir = std::filesystem::temp_directory_path() / L"DDCCIPluginTests";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

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
  auto hugeNumber =
      BrightnessPresetsConfig::ParsePresetText(L"999999999999999999999");

  assert(!invalidToken.valid);
  assert(invalidToken.presets.empty());
  assert(!outOfRange.valid);
  assert(outOfRange.presets.empty());
  assert(!empty.valid);
  assert(empty.presets.empty());
  assert(!hugeNumber.valid);
  assert(hugeNumber.presets.empty());
}

static void Test_BrightnessPresetFormatter_FormatsSpaceSeparatedText() {
  std::vector<int> presets{0, 25, 50, 75, 100};

  assert(BrightnessPresetsConfig::FormatPresetText(presets) ==
         L"0 25 50 75 100");
}

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

static void Test_DataRequired_ReadsUntilFirstSuccessfulBrightness() {
  ResetControllerState();
  Plugin::SetTickCountProviderForTest(TestGetTickCount);
  Plugin plugin;

  plugin.DataRequired();
  assert(g_getBrightnessCalls == 1);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"N/A") == 0);

  plugin.DataRequired();
  assert(g_getBrightnessCalls == 2);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"42%") == 0);

  plugin.DataRequired();
  assert(g_getBrightnessCalls == 2);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"42%") == 0);
}

static void Test_CommandBrightnessDelaysReadbackWithoutBlocking() {
  ResetControllerState();
  Plugin::SetTickCountProviderForTest(TestGetTickCount);
  Plugin plugin;

  plugin.OnPluginCommand(2, nullptr, nullptr);
  assert(g_setBrightnessCalls == 1);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"50%") == 0);

  g_currentTick = 99;
  plugin.DataRequired();
  assert(g_getBrightnessCalls == 0);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"50%") == 0);

  g_currentTick = 100;
  plugin.DataRequired();
  assert(g_getBrightnessCalls == 1);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"50%") == 0);

  g_currentTick = 200;
  plugin.DataRequired();
  assert(g_getBrightnessCalls == 2);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"42%") == 0);
}

static void Test_CommandBrightnessReadbackHandlesTickWraparound() {
  ResetControllerState();
  Plugin::SetTickCountProviderForTest(TestGetTickCount);
  Plugin plugin;

  g_currentTick = 0xFFFFFFF0;
  plugin.OnPluginCommand(2, nullptr, nullptr);
  assert(std::wcscmp(plugin.GetItem(0)->GetItemValueText(), L"50%") == 0);

  plugin.DataRequired();
  assert(g_getBrightnessCalls == 0);

  g_currentTick = 50;
  plugin.DataRequired();
  assert(g_getBrightnessCalls == 0);

  g_currentTick = 84;
  plugin.DataRequired();
  assert(g_getBrightnessCalls == 1);
}

static void Test_PluginInfo_Version() {
  Plugin plugin;

  assert(std::wcscmp(plugin.GetInfo(ITMPlugin::TMI_VERSION), L"1.2.0") == 0);
}

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

static void Test_Plugin_SaveBeforeConfigDirLeavesDefaultsUnchanged() {
  Plugin plugin;

  auto result = plugin.SavePresetTextForTest(L"15 45 90");

  assert(result == ITMPlugin::OR_OPTION_UNCHANGED);
  assert(plugin.GetCommandCount() == 7);
  assert(std::wcscmp(plugin.GetCommandName(0), L"亮度 0%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(1), L"亮度 25%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(2), L"亮度 50%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(3), L"亮度 75%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(4), L"亮度 100%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(5), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(6), L"电源 关机") == 0);
}

int RunTests() {
  Test_BrightnessPresetParser_ParsesDefaults();
  Test_BrightnessPresetParser_DeduplicatesInInputOrder();
  Test_BrightnessPresetParser_InvalidTextClearsList();
  Test_BrightnessPresetFormatter_FormatsSpaceSeparatedText();
  Test_BrightnessPresetConfig_InitializesMissingFile();
  Test_BrightnessPresetConfig_LoadsExistingFile();
  Test_BrightnessPresetConfig_SavesText();
  Test_PluginInfo_Version();
  Test_CommandNames_UseDefaultDynamicPresets();
  Test_CommandBrightness_UsesPresetValue();
  Test_Plugin_LoadsPresetsFromConfigDirectory();
  Test_Plugin_InvalidConfigShowsOnlyPowerCommands();
  Test_Plugin_SavePresetTextUpdatesCommandsAndFile();
  Test_Plugin_SaveInvalidPresetTextKeepsOnlyPowerCommands();
  Test_Plugin_SaveBeforeConfigDirLeavesDefaultsUnchanged();
  Test_DataRequired_ReadsUntilFirstSuccessfulBrightness();
  Test_CommandBrightnessDelaysReadbackWithoutBlocking();
  Test_CommandBrightnessReadbackHandlesTickWraparound();
  Plugin::SetTickCountProviderForTest(nullptr);
  return 0;
}

int main() {
  try {
    return RunTests();
  } catch (...) {
    return 1;
  }
}
