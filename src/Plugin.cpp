#include "Plugin.h"
#include "MonitorController.h"

int Plugin::GetAPIVersion() const { return 7; }

IPluginItem *Plugin::GetItem(int index) {
  if (index == 0)
    return &m_brightnessItem;
  return nullptr;
}

void Plugin::DataRequired() {}

const wchar_t *Plugin::GetInfo(PluginInfoIndex index) {
  switch (index) {
  case TMI_NAME:
    return L"DDC/CI 显示器亮度控制";
  case TMI_DESCRIPTION:
    return L"通过 DDC/CI 协议控制显示器亮度";
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

int Plugin::GetCommandCount() { return 12; }

const wchar_t *Plugin::GetCommandName(int command_index) {
  switch (command_index) {
  case 0:
    return L"关闭显示器";
  case 1:
    return L"亮度 0";
  case 2:
    return L"亮度 10";
  case 3:
    return L"亮度 20";
  case 4:
    return L"亮度 30";
  case 5:
    return L"亮度 40";
  case 6:
    return L"亮度 50";
  case 7:
    return L"亮度 60";
  case 8:
    return L"亮度 70";
  case 9:
    return L"亮度 80";
  case 10:
    return L"亮度 90";
  case 11:
    return L"亮度 100";
  default:
    return nullptr;
  }
}

void Plugin::OnPluginCommand(int command_index, void *hWnd, void *para) {
  (void)hWnd;
  (void)para;

  if (command_index == 0) {
    MonitorController::TurnOff();
  } else if (command_index >= 1 && command_index <= 11) {
    int brightness = (command_index - 1) * 10;
    int actual = MonitorController::SetAndGetBrightness(brightness);
    if (actual >= 0) {
      m_brightnessItem.UpdateBrightness(actual);
    }
  }
}

void Plugin::OnInitialize(ITrafficMonitor *pApp) {
  m_pApp = pApp;
  RefreshBrightnessDisplay();
}

void Plugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) {
  switch (index) {
  case EI_MAIN_WND_NOT_SHOW_PERCENT:
    m_noPercent = (data && wcscmp(data, L"1") == 0);
    m_brightnessItem.SetFormatOptions(m_noPercent, m_spaceBeforeUnit);
    break;
  case EI_MAIN_WND_SPERATE_WITH_SPACE:
    m_spaceBeforeUnit = (data && wcscmp(data, L"1") == 0);
    m_brightnessItem.SetFormatOptions(m_noPercent, m_spaceBeforeUnit);
    break;
  default:
    break;
  }
}

void Plugin::RefreshBrightnessDisplay() {
  int brightness = MonitorController::GetBrightness();
  m_brightnessItem.UpdateBrightness(brightness);
}