#include "Plugin.h"
#include "MonitorController.h"
#include <cwchar>

static constexpr DWORD kBrightnessReadbackDelayMs = 100;
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

int Plugin::GetCommandCount() { return 13; }

const wchar_t *Plugin::GetCommandName(int command_index) {
  switch (command_index) {
  case 0:
    return L"亮度 0%";
  case 1:
    return L"亮度 10%";
  case 2:
    return L"亮度 20%";
  case 3:
    return L"亮度 30%";
  case 4:
    return L"亮度 40%";
  case 5:
    return L"亮度 50%";
  case 6:
    return L"亮度 60%";
  case 7:
    return L"亮度 70%";
  case 8:
    return L"亮度 80%";
  case 9:
    return L"亮度 90%";
  case 10:
    return L"亮度 100%";
  case 11:
    return L"电源 待机";
  case 12:
    return L"电源 关机";
  default:
    return L"";
  }
}

void Plugin::OnPluginCommand(int command_index, void *hWnd, void *para) {
  (void)hWnd;
  (void)para;

  if (command_index >= 0 && command_index <= 10) {
    int value = command_index * 10;
    if (MonitorController::SetBrightness(value)) {
      m_brightnessItem.UpdateBrightness(value);
      m_brightnessNeedsUpdate = true;
      m_nextBrightnessUpdateTick =
          CurrentTickCount() + kBrightnessReadbackDelayMs;
    }
  } else if (command_index == 11) {
    MonitorController::Standby();
  } else if (command_index == 12) {
    MonitorController::TurnOff();
  }
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
