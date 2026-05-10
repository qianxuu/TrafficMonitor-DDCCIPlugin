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

int main() {
  Test_DataRequired_ReadsUntilFirstSuccessfulBrightness();
  Test_CommandBrightnessDelaysReadbackWithoutBlocking();
  Test_CommandBrightnessReadbackHandlesTickWraparound();
  Plugin::SetTickCountProviderForTest(nullptr);
  return 0;
}
