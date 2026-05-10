#include "BrightnessPresetsConfig.h"
#include "MonitorController.h"
#include "Plugin.h"
#include <cassert>
#include <cwchar>

static int g_getBrightnessCalls = 0;
static int g_setBrightnessCalls = 0;
static DWORD g_currentTick = 0;
static int g_brightnessValues[] = {-1, 42, 80};

static void ResetControllerState() {
  g_getBrightnessCalls = 0;
  g_setBrightnessCalls = 0;
  g_currentTick = 0;
}

DWORD TestGetTickCount() { return g_currentTick; }

bool MonitorController::Standby() { return true; }

bool MonitorController::TurnOff() { return true; }

bool MonitorController::SetBrightness(int value) {
  ++g_setBrightnessCalls;
  (void)value;
  return true;
}

int MonitorController::GetBrightness() {
  int index = g_getBrightnessCalls;
  ++g_getBrightnessCalls;
  return g_brightnessValues[index];
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

  plugin.OnPluginCommand(5, nullptr, nullptr);
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
  plugin.OnPluginCommand(5, nullptr, nullptr);
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

static void Test_CommandNames_AreStable() {
  Plugin plugin;

  assert(plugin.GetCommandCount() == 13);
  assert(std::wcscmp(plugin.GetCommandName(-1), L"") == 0);
  assert(std::wcscmp(plugin.GetCommandName(0), L"亮度 0%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(5), L"亮度 50%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(10), L"亮度 100%") == 0);
  assert(std::wcscmp(plugin.GetCommandName(11), L"电源 待机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(12), L"电源 关机") == 0);
  assert(std::wcscmp(plugin.GetCommandName(13), L"") == 0);
}

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
