#include "Plugin.h"
#include "BrightnessPresetsConfig.h"
#include "MonitorController.h"
#include <commctrl.h>
#include <cwchar>
#include <string>

static constexpr DWORD kBrightnessReadbackDelayMs = 100;
static constexpr const wchar_t *kStandbyCommandName = L"电源 待机";
static constexpr const wchar_t *kTurnOffCommandName = L"电源 关机";
static Plugin::TickCountProvider g_tickCountProvider = GetTickCount;

namespace {

constexpr wchar_t kOptionsDialogClassName[] = L"DDCCIPluginOptionsDialog";
constexpr int kPresetEditId = 1001;
constexpr int kPresetBufferSize = 256;

struct OptionsDialogState {
  HWND edit = nullptr;
  bool done = false;
  bool accepted = false;
  wchar_t text[kPresetBufferSize] = {};
};

LRESULT CALLBACK OptionsDialogProc(HWND hwnd, UINT message, WPARAM wParam,
                                   LPARAM lParam) {
  auto *state = reinterpret_cast<OptionsDialogState *>(
      GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  if (message == WM_NCCREATE) {
    auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
    state = static_cast<OptionsDialogState *>(create->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
  }

  switch (message) {
  case WM_COMMAND:
    if (state != nullptr) {
      int command = LOWORD(wParam);
      if (command == IDOK) {
        GetWindowTextW(state->edit, state->text, kPresetBufferSize);
        state->accepted = true;
        state->done = true;
        DestroyWindow(hwnd);
        return 0;
      }
      if (command == IDCANCEL) {
        state->done = true;
        DestroyWindow(hwnd);
        return 0;
      }
    }
    break;
  case WM_CLOSE:
    if (state != nullptr) {
      state->done = true;
    }
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    if (state != nullptr) {
      state->done = true;
    }
    return 0;
  default:
    break;
  }

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

ATOM RegisterOptionsDialogClass(HINSTANCE instance) {
  WNDCLASSW windowClass = {};
  windowClass.lpfnWndProc = OptionsDialogProc;
  windowClass.hInstance = instance;
  windowClass.lpszClassName = kOptionsDialogClassName;
  windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
  return RegisterClassW(&windowClass);
}

} // namespace

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
    m_brightnessCommandNames.push_back(L"亮度 " + std::to_wstring(preset) + L"%");
  }
}

Plugin::Plugin() { ApplyPresetText(BrightnessPresetsConfig::DefaultPresetText()); }

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

ITMPlugin::OptionReturn Plugin::SavePresetTextForTest(const std::wstring &text) {
  return SavePresetText(text);
}

ITMPlugin::OptionReturn Plugin::ShowOptionsDialog(void *hParent) {
  HWND parent = static_cast<HWND>(hParent);
  HINSTANCE instance = GetModuleHandleW(nullptr);
  RegisterOptionsDialogClass(instance);

  OptionsDialogState state;
  wcsncpy_s(state.text, m_presetText.c_str(), _TRUNCATE);

  HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kOptionsDialogClassName,
                                L"显示器控制设置",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 360, 150, parent,
                                nullptr, instance, &state);
  if (dialog == nullptr) {
    return OR_OPTION_UNCHANGED;
  }

  CreateWindowExW(0, L"STATIC", L"亮度预设（空格分隔，0 到 100）：",
                  WS_CHILD | WS_VISIBLE, 12, 14, 320, 20, dialog, nullptr,
                  instance, nullptr);
  state.edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state.text,
                               WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 12, 40,
                               320, 24, dialog,
                               reinterpret_cast<HMENU>(
                                   static_cast<INT_PTR>(kPresetEditId)),
                               instance, nullptr);
  CreateWindowExW(0, L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE, 176, 80, 72,
                  24, dialog, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
  CreateWindowExW(0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE, 260, 80, 72,
                  24, dialog, reinterpret_cast<HMENU>(IDCANCEL), instance,
                  nullptr);

  if (parent != nullptr) {
    EnableWindow(parent, FALSE);
  }
  ShowWindow(dialog, SW_SHOW);
  SetFocus(state.edit);

  MSG msg;
  while (!state.done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageW(dialog, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  if (parent != nullptr) {
    EnableWindow(parent, TRUE);
  }
  if (parent != nullptr) {
    SetActiveWindow(parent);
  }

  if (state.accepted) {
    return SavePresetText(state.text);
  }
  return OR_OPTION_UNCHANGED;
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
