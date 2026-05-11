#pragma once

#include "BrightnessItem.h"
#include "PluginInterface.h"
#include <string>
#include <vector>
#include <windows.h>

class Plugin : public ITMPlugin {
public:
  using TickCountProvider = DWORD(WINAPI *)();

  Plugin();

  // ITMPlugin
  int GetAPIVersion() const override;
  IPluginItem *GetItem(int index) override;
  void DataRequired() override;
  const wchar_t *GetInfo(PluginInfoIndex index) override;
  int GetCommandCount() override;
  const wchar_t *GetCommandName(int command_index) override;
  void OnPluginCommand(int command_index, void *hWnd, void *para) override;
  OptionReturn ShowOptionsDialog(void *hParent) override;
  void OnInitialize(ITrafficMonitor *pApp) override;
  void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;

  static void SetTickCountProviderForTest(TickCountProvider provider);

  OptionReturn SavePresetTextForTest(const std::wstring &text);

private:
  OptionReturn SavePresetText(const std::wstring &text);
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
  std::wstring m_configDir;
  std::wstring m_presetText;
  std::vector<int> m_brightnessPresets;
  std::vector<std::wstring> m_brightnessCommandNames;

  void ApplyDisplayOptions();
  void ApplyPresetText(const std::wstring &text);
};
