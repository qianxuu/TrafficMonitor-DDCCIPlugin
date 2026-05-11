# Options Dialog Extraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the Win32 options dialog implementation out of `Plugin.cpp` into a focused `OptionsDialog` module without changing plugin behavior.

**Architecture:** `Plugin` remains responsible for TrafficMonitor plugin behavior and saving preset text. A new pure Win32 `OptionsDialog` module owns window class registration, dialog controls, font setup, work-area centering, and the modal message loop. The boundary is a small result-returning function that takes parent window and current preset text.

**Tech Stack:** C++23, Win32 API, CMake, clang-format, clang-tidy, existing assert-based CTest tests.

---

## File Structure

- Create `src/OptionsDialog.h`: declares `OptionsDialogResult` and `ShowBrightnessPresetOptionsDialog(HWND, const std::wstring&)`.
- Create `src/OptionsDialog.cpp`: contains all current options dialog Win32 implementation details.
- Modify `src/Plugin.cpp`: remove dialog-specific helpers and delegate `ShowOptionsDialog()` to `ShowBrightnessPresetOptionsDialog()`.
- Modify `CMakeLists.txt`: add `src/OptionsDialog.cpp` to the `DDCCIPlugin` target and `test_plugin` target.
- Keep `src/Plugin.h` unchanged unless the compiler requires include cleanup.
- Keep tests behavior-focused; no automated UI invocation because modal UI can block.

---

### Task 1: Extract Options Dialog Module

**Files:**
- Create: `src/OptionsDialog.h`
- Create: `src/OptionsDialog.cpp`
- Modify: `src/Plugin.cpp`
- Modify: `CMakeLists.txt`
- Test: existing `test/test_plugin.cpp` and `test/test_brightness_item.cpp`

- [ ] **Step 1: Run baseline tests before refactor**

Run:

```bash
cmake --build build && ctest --test-dir build -C Debug --output-on-failure
```

Expected: PASS, `2/2` tests passed. This establishes that existing behavior is green before moving code.

- [ ] **Step 2: Create the dialog interface header**

Create `src/OptionsDialog.h`:

```cpp
#pragma once

#include <string>
#include <windows.h>

struct OptionsDialogResult {
  bool accepted = false;
  std::wstring presetText;
};

OptionsDialogResult ShowBrightnessPresetOptionsDialog(
    HWND parent, const std::wstring &currentPresetText);
```

- [ ] **Step 3: Move dialog implementation into `OptionsDialog.cpp`**

Create `src/OptionsDialog.cpp` with the dialog-only code currently in `src/Plugin.cpp`:

```cpp
#include "OptionsDialog.h"

#include <commctrl.h>
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

} // namespace
```

Then copy the current implementations of these functions from `src/Plugin.cpp` into `OptionsDialog.cpp`:

- `CreateDialogFont()`
- `SetControlFont(HWND, HFONT)`
- `CenterWindow(HWND, HWND)`
- `OptionsDialogProc(HWND, UINT, WPARAM, LPARAM)`
- `RegisterOptionsDialogClass(HINSTANCE)`

Finally add this public function at the bottom of `OptionsDialog.cpp`:

```cpp
OptionsDialogResult ShowBrightnessPresetOptionsDialog(
    HWND parent, const std::wstring &currentPresetText) {
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
```

- [ ] **Step 4: Simplify `Plugin.cpp`**

In `src/Plugin.cpp`, add:

```cpp
#include "OptionsDialog.h"
```

Remove dialog-only code from `src/Plugin.cpp`:

- `#include <commctrl.h>` if no longer needed
- `kOptionsDialogClassName`
- `kPresetEditId`
- `kPresetBufferSize`
- dialog layout constants
- `DialogFont`
- `CreateDialogFont()`
- `SetControlFont()`
- `CenterWindow()`
- `OptionsDialogState`
- `OptionsDialogProc()`
- `RegisterOptionsDialogClass()`

Replace `Plugin::ShowOptionsDialog(void *hParent)` with:

```cpp
ITMPlugin::OptionReturn Plugin::ShowOptionsDialog(void *hParent) {
  auto result = ShowBrightnessPresetOptionsDialog(static_cast<HWND>(hParent),
                                                  m_presetText);
  if (!result.accepted) {
    return OR_OPTION_UNCHANGED;
  }

  return SavePresetText(result.presetText);
}
```

- [ ] **Step 5: Wire `OptionsDialog.cpp` into CMake**

Modify the plugin target in `CMakeLists.txt`:

```cmake
add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
    src/MonitorController.cpp
    src/OptionsDialog.cpp
)
```

Modify the `test_plugin` target:

```cmake
add_executable(test_plugin
    test/test_plugin.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/BrightnessPresetsConfig.cpp
    src/OptionsDialog.cpp
)
```

- [ ] **Step 6: Format, lint, and run tests**

Run:

```bash
clang-format -i src/BrightnessPresetsConfig.cpp src/Plugin.cpp src/dllmain.cpp src/OptionsDialog.h src/OptionsDialog.cpp test/test_plugin.cpp
```

Run:

```bash
clang-tidy -header-filter='^(?!.*include[\\/]PluginInterface\.h$).*$' src/BrightnessItem.cpp src/BrightnessPresetsConfig.cpp src/MonitorController.cpp src/Plugin.cpp src/dllmain.cpp src/OptionsDialog.cpp test/test_brightness_item.cpp test/test_plugin.cpp -- -std=c++23 -Iinclude -Isrc -DWIN32_LEAN_AND_MEAN
```

Expected: no visible warnings from project code; warnings from `include/PluginInterface.h` are intentionally excluded because it is an external ABI header.

Run:

```bash
cmake --build build && ctest --test-dir build -C Debug --output-on-failure
```

Expected: PASS, `2/2` tests passed.

- [ ] **Step 7: Review the diff**

Run:

```bash
git diff --stat && git diff --check
```

Expected:

- `Plugin.cpp` loses dialog-specific code.
- `OptionsDialog.h/.cpp` contains the moved dialog implementation.
- `CMakeLists.txt` includes `src/OptionsDialog.cpp`.
- `git diff --check` reports no whitespace errors.

- [ ] **Step 8: Commit the refactor**

```bash
git add CMakeLists.txt src/Plugin.cpp src/OptionsDialog.h src/OptionsDialog.cpp
git commit -m "refactor: extract options dialog module"
```

---

## Self-Review

- Spec coverage: extracts the settings/options dialog code from `Plugin.cpp` into `OptionsDialog.h/.cpp` without changing behavior.
- Placeholder scan: no placeholder steps remain.
- Type consistency: `OptionsDialogResult` and `ShowBrightnessPresetOptionsDialog(HWND, const std::wstring&)` are declared in the header and used by `Plugin.cpp`.
- Scope check: no MFC migration, no resource dialog conversion, no command/config logic changes, and no automated modal UI invocation.
