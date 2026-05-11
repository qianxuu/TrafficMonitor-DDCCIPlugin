#include "OptionsDialog.h"

#include <cwchar>

namespace {

constexpr wchar_t kOptionsDialogClassName[] = L"DDCCIPluginOptionsDialog";
constexpr int kPresetEditId = 1001;
constexpr int kPresetBufferSize = 256;
constexpr int kOptionsDialogWidth = 360;
constexpr int kOptionsDialogHeight = 150;
constexpr int kDialogMargin = 12;
constexpr int kLabelTop = 14;
constexpr int kLabelHeight = 20;
constexpr int kEditTop = 40;
constexpr int kEditHeight = 24;
constexpr int kButtonTop = 80;
constexpr int kButtonWidth = 72;
constexpr int kButtonHeight = 24;
constexpr int kOkButtonLeft = 176;
constexpr int kCancelButtonLeft = 260;

struct DialogFont {
  HFONT handle = nullptr;
  bool owned = false;
};

struct OptionsDialogState {
  HWND edit = nullptr;
  bool done = false;
  bool accepted = false;
  wchar_t text[kPresetBufferSize] = {};
};

DialogFont CreateDialogFont();
void SetControlFont(HWND hwnd, HFONT font);
void CenterWindow(HWND window, HWND parent);
LRESULT CALLBACK OptionsDialogProc(HWND hwnd, UINT message, WPARAM wParam,
                                   LPARAM lParam);
ATOM RegisterOptionsDialogClass(HINSTANCE instance);

DialogFont CreateDialogFont() {
  NONCLIENTMETRICSW metrics = {};
  metrics.cbSize = sizeof(metrics);
  if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics,
                            0)) {
    if (HFONT font = CreateFontIndirectW(&metrics.lfMessageFont)) {
      return {font, true};
    }
  }

  return {static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)), false};
}

void SetControlFont(HWND hwnd, HFONT font) {
  if (hwnd != nullptr && font != nullptr) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  }
}

void CenterWindow(HWND window, HWND parent) {
  RECT windowRect = {};
  GetWindowRect(window, &windowRect);
  int width = windowRect.right - windowRect.left;
  int height = windowRect.bottom - windowRect.top;

  HMONITOR monitor = parent != nullptr && IsWindow(parent)
                         ? MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST)
                         : MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

  MONITORINFO monitorInfo = {};
  monitorInfo.cbSize = sizeof(monitorInfo);
  RECT targetRect = {};
  if (monitor != nullptr && GetMonitorInfoW(monitor, &monitorInfo)) {
    targetRect = monitorInfo.rcWork;
  } else {
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &targetRect, 0);
  }

  int left =
      targetRect.left + (((targetRect.right - targetRect.left) - width) / 2);
  int top =
      targetRect.top + (((targetRect.bottom - targetRect.top) - height) / 2);
  SetWindowPos(window, nullptr, left, top, 0, 0,
               SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

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

OptionsDialogResult
ShowBrightnessPresetOptionsDialog(HWND parent,
                                  const std::wstring &currentPresetText) {
  HINSTANCE instance = GetModuleHandleW(nullptr);
  RegisterOptionsDialogClass(instance);

  OptionsDialogState state;
  wcsncpy_s(state.text, currentPresetText.c_str(), _TRUNCATE);

  HWND dialog =
      CreateWindowExW(WS_EX_DLGMODALFRAME, kOptionsDialogClassName,
                      L"显示器控制设置", WS_POPUP | WS_CAPTION | WS_SYSMENU,
                      CW_USEDEFAULT, CW_USEDEFAULT, kOptionsDialogWidth,
                      kOptionsDialogHeight, parent, nullptr, instance, &state);
  if (dialog == nullptr) {
    return {};
  }

  HWND label = CreateWindowExW(
      0, L"STATIC", L"亮度预设（空格分隔，0 到 100）：", WS_CHILD | WS_VISIBLE,
      kDialogMargin, kLabelTop, kOptionsDialogWidth - (2 * kDialogMargin),
      kLabelHeight, dialog, nullptr, instance, nullptr);
  state.edit = CreateWindowExW(
      WS_EX_CLIENTEDGE, L"EDIT", state.text,
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, kDialogMargin,
      kEditTop, kOptionsDialogWidth - (2 * kDialogMargin), kEditHeight, dialog,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPresetEditId)), instance,
      nullptr);
  HWND okButton =
      CreateWindowExW(0, L"BUTTON", L"确定",
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                      kOkButtonLeft, kButtonTop, kButtonWidth, kButtonHeight,
                      dialog, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
  HWND cancelButton = CreateWindowExW(
      0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
      kCancelButtonLeft, kButtonTop, kButtonWidth, kButtonHeight, dialog,
      reinterpret_cast<HMENU>(IDCANCEL), instance, nullptr);

  if (label == nullptr || state.edit == nullptr || okButton == nullptr ||
      cancelButton == nullptr) {
    DestroyWindow(dialog);
    return {};
  }

  DialogFont dialogFont = CreateDialogFont();
  SetControlFont(dialog, dialogFont.handle);
  SetControlFont(label, dialogFont.handle);
  SetControlFont(state.edit, dialogFont.handle);
  SetControlFont(okButton, dialogFont.handle);
  SetControlFont(cancelButton, dialogFont.handle);
  CenterWindow(dialog, parent);

  if (parent != nullptr) {
    EnableWindow(parent, FALSE);
  }
  ShowWindow(dialog, SW_SHOW);
  SetFocus(state.edit);

  MSG msg;
  BOOL getMessageResult = TRUE;
  while (!state.done &&
         (getMessageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
    if (!IsDialogMessageW(dialog, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  const bool shouldRepostQuit = getMessageResult == 0;
  const int quitExitCode = shouldRepostQuit ? static_cast<int>(msg.wParam) : 0;

  if (IsWindow(dialog)) {
    DestroyWindow(dialog);
  }

  if (parent != nullptr) {
    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
  }

  if (dialogFont.owned) {
    DeleteObject(dialogFont.handle);
  }

  if (shouldRepostQuit) {
    PostQuitMessage(quitExitCode);
  }

  return {state.accepted, state.accepted ? std::wstring(state.text) : L""};
}
