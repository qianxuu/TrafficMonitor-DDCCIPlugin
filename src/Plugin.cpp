#include "Plugin.h"
#include "BrightnessPresetsConfig.h"
#include "MonitorController.h"
#include "OptionsDialog.h"
#include <algorithm>
#include <cwchar>
#include <string>

static constexpr DWORD kBrightnessReadbackDelayMs = 100;
static constexpr const wchar_t *kStandbyCommandName = L"电源 待机";
static constexpr const wchar_t *kTurnOffCommandName = L"电源 关机";
static Plugin::TickCountProvider g_tickCountProvider = GetTickCount;

static bool ParseBoolData(const wchar_t *data) {
  if (data == nullptr || data[0] == L'\0')
    return false;
  return std::wcscmp(data, L"1") == 0;
}

static DWORD CurrentTickCount() { return g_tickCountProvider(); }

static bool HasTickReached(DWORD current, DWORD target) {
  return static_cast<LONG>(current - target) >= 0;
}

void Plugin::SetTickCountProviderForTest(TickCountProvider provider) {
  g_tickCountProvider = provider ? provider : GetTickCount;
}

void Plugin::ApplyPresetText(const std::wstring &text) {
  m_presetText = text;
  auto parsed = BrightnessPresetsConfig::ParsePresetText(text);
  m_brightnessPresets = parsed.presets;
  m_brightnessCommandNames.clear();

  for (int preset : m_brightnessPresets) {
    m_brightnessCommandNames.push_back(L"亮度 " + std::to_wstring(preset) +
                                       L"%");
  }
}

Plugin::Plugin() {
  ApplyPresetText(BrightnessPresetsConfig::DefaultPresetText());
}

int Plugin::GetAPIVersion() const { return 7; }

IPluginItem *Plugin::GetItem(int index) {
  if (index == 0)
    return &m_brightnessItem;
  return nullptr;
}

void Plugin::DataRequired() {
  if (!m_brightnessNeedsUpdate) {
    return;
  }

  if (!HasTickReached(CurrentTickCount(), m_nextBrightnessUpdateTick)) {
    return;
  }

  int brightness = MonitorController::GetBrightness();
  if (brightness >= 0) {
    m_brightnessItem.UpdateBrightness(brightness);
    m_brightnessNeedsUpdate = false;
  }
}

const wchar_t *Plugin::GetInfo(PluginInfoIndex index) {
  switch (index) {
  case TMI_NAME:
    return L"显示器控制";
  case TMI_DESCRIPTION:
    return L"通过 DDC/CI 协议控制显示器";
  case TMI_AUTHOR:
    return L"qianxu";
  case TMI_COPYRIGHT:
    return L"Copyright (C) by qianxu 2026";
  case TMI_VERSION:
    return L"1.1.0";
  case TMI_URL:
    return L"https://github.com/qianxuu";
  default:
    return L"";
  }
}

int Plugin::GetCommandCount() {
  return static_cast<int>(m_brightnessPresets.size()) + 2;
}

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

void Plugin::OnPluginCommand(int command_index, void *hWnd, void *para) {
  (void)hWnd;
  (void)para;

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
}

ITMPlugin::OptionReturn Plugin::SavePresetText(const std::wstring &text) {
  if (m_configDir.empty()) {
    return OR_OPTION_UNCHANGED;
  }

  if (!BrightnessPresetsConfig::SavePresetText(m_configDir, text)) {
    return OR_OPTION_UNCHANGED;
  }

  ApplyPresetText(text);
  return OR_OPTION_CHANGED;
}

ITMPlugin::OptionReturn
Plugin::SavePresetTextForTest(const std::wstring &text) {
  return SavePresetText(text);
}

ITMPlugin::OptionReturn Plugin::ShowOptionsDialog(void *hParent) {
  auto result = ShowBrightnessPresetOptionsDialog(static_cast<HWND>(hParent),
                                                  m_presetText);
  if (!result.accepted) {
    return OR_OPTION_UNCHANGED;
  }

  return SavePresetText(result.presetText);
}

void Plugin::OnInitialize(ITrafficMonitor *pApp) { m_pApp = pApp; }

void Plugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) {
  switch (index) {
  case EI_DRAW_TASKBAR_WND:
    m_drawTaskbarWnd = ParseBoolData(data);
    break;
  case EI_MAIN_WND_NOT_SHOW_PERCENT:
    m_mainNoPercent = ParseBoolData(data);
    m_hasMainNoPercent = true;
    break;
  case EI_MAIN_WND_SPERATE_WITH_SPACE:
    m_mainSpaceBeforeUnit = ParseBoolData(data);
    m_hasMainSpaceBeforeUnit = true;
    break;
  case EI_TASKBAR_WND_NOT_SHOW_PERCENT:
    m_taskbarNoPercent = ParseBoolData(data);
    m_hasTaskbarNoPercent = true;
    break;
  case EI_TASKBAR_WND_SPERATE_WITH_SPACE:
    m_taskbarSpaceBeforeUnit = ParseBoolData(data);
    m_hasTaskbarSpaceBeforeUnit = true;
    break;
  case EI_CONFIG_DIR:
    if (data != nullptr) {
      m_configDir = data;
      auto config = BrightnessPresetsConfig::LoadOrInitialize(m_configDir);
      ApplyPresetText(config.text);
    }
    break;
  default:
    break;
  }

  ApplyDisplayOptions();
}

void Plugin::ApplyDisplayOptions() {
  bool noPercent;
  bool spaceBeforeUnit;

  if (m_drawTaskbarWnd) {
    noPercent = m_hasTaskbarNoPercent ? m_taskbarNoPercent : false;
    spaceBeforeUnit =
        m_hasTaskbarSpaceBeforeUnit ? m_taskbarSpaceBeforeUnit : false;
  } else {
    noPercent = m_hasMainNoPercent ? m_mainNoPercent : false;
    spaceBeforeUnit = m_hasMainSpaceBeforeUnit ? m_mainSpaceBeforeUnit : false;
  }

  m_brightnessItem.SetFormatOptions(noPercent, spaceBeforeUnit);
}
