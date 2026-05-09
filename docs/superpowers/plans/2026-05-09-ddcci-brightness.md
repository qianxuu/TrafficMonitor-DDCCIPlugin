# DDC/CI 显示器亮度控制插件 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the DDCCIPlugin with a brightness display item and brightness/off menu commands via DDC/CI protocol.

**Architecture:** Add a `BrightnessItem` class implementing `IPluginItem` for text display, extend `MonitorController` with `SetBrightness`/`GetBrightness` using Windows high-level API, and update `Plugin` to wire commands and format brightness text per TrafficMonitor display settings.

**Tech Stack:** C++17, Windows SDK (dxva2.lib, highlevelmonitorconfigurationapi.h), MSVC 2022, CMake

---

## File Layout

```
DDCCIPlugin/
├── include/
│   └── PluginInterface.h           (existing — no changes)
├── src/
│   ├── dllmain.cpp                 (existing — no changes)
│   ├── Plugin.def                  (existing — no changes)
│   ├── Plugin.h                    (modify: add BrightnessItem member, OnExtenedInfo)
│   ├── Plugin.cpp                  (modify: GetItem, commands, format logic)
│   ├── BrightnessItem.h            (create)
│   ├── BrightnessItem.cpp          (create)
│   ├── MonitorController.h         (modify: add SetBrightness, GetBrightness)
│   └── MonitorController.cpp       (modify: implement brightness read/write)
├── CMakeLists.txt                  (modify: add BrightnessItem.cpp)
└── .clang-tidy                     (existing — no changes)
```

---

### Task 1: Add brightness methods to MonitorController

**Files:**
- Modify: `src/MonitorController.h`
- Modify: `src/MonitorController.cpp`

- [ ] **Step 1: Update `src/MonitorController.h`** — add brightness declarations

```cpp
#pragma once

#include <windows.h>

class MonitorController {
public:
  static bool TurnOff();
  static bool SetBrightness(int value);
  static int  GetBrightness();

private:
  static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                       LPRECT lprcClip, LPARAM dwData);
  static HANDLE OpenFirstPhysicalMonitor();
  static void   ClosePhysicalMonitor(HANDLE hMonitor);
};
```

- [ ] **Step 2: Update `src/MonitorController.cpp`** — refactor common monitor acquisition, add brightness methods

```cpp
#include "MonitorController.h"
#include <highlevelmonitorconfigurationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <windows.h>

#pragma comment(lib, "dxva2.lib")

struct EnumContext {
  HMONITOR hResult = nullptr;
};

BOOL CALLBACK MonitorController::MonitorEnumProc(HMONITOR hMonitor,
                                                 HDC hdcMonitor,
                                                 LPRECT lprcClip,
                                                 LPARAM dwData) {
  (void)hdcMonitor;
  (void)lprcClip;

  auto *ctx = reinterpret_cast<EnumContext *>(
      dwData); // NOLINT: Win32 LPARAM ↔ pointer cast
  ctx->hResult = hMonitor;
  return FALSE;
}

HANDLE MonitorController::OpenFirstPhysicalMonitor() {
  EnumContext ctx;
  HDC hdc = GetDC(nullptr);
  EnumDisplayMonitors(hdc, nullptr, MonitorEnumProc,
                      reinterpret_cast<LPARAM>(&ctx));
  ReleaseDC(nullptr, hdc);

  if (!ctx.hResult)
    return nullptr;

  DWORD cPhysicalMonitors = 0;
  if (!GetNumberOfPhysicalMonitorsFromHMONITOR(ctx.hResult,
                                                &cPhysicalMonitors) ||
      cPhysicalMonitors == 0)
    return nullptr;

  PHYSICAL_MONITOR *pPhysicalMonitors = new PHYSICAL_MONITOR[cPhysicalMonitors];
  if (!GetPhysicalMonitorsFromHMONITOR(ctx.hResult, cPhysicalMonitors,
                                       pPhysicalMonitors)) {
    delete[] pPhysicalMonitors;
    return nullptr;
  }

  HANDLE hPhysicalMonitor = pPhysicalMonitors[0].hPhysicalMonitor;
  delete[] pPhysicalMonitors;
  return hPhysicalMonitor;
}

void MonitorController::ClosePhysicalMonitor(HANDLE hMonitor) {
  if (hMonitor) {
    DestroyPhysicalMonitor(hMonitor);
  }
}

bool MonitorController::TurnOff() {
  HANDLE hMonitor = OpenFirstPhysicalMonitor();
  if (!hMonitor)
    return false;

  bool success = SetVCPFeature(hMonitor, 0xD6, 0x04) != 0;

  ClosePhysicalMonitor(hMonitor);
  return success;
}

bool MonitorController::SetBrightness(int value) {
  HANDLE hMonitor = OpenFirstPhysicalMonitor();
  if (!hMonitor)
    return false;

  bool success = SetMonitorBrightness(hMonitor, static_cast<DWORD>(value)) != 0;

  ClosePhysicalMonitor(hMonitor);
  return success;
}

int MonitorController::GetBrightness() {
  HANDLE hMonitor = OpenFirstPhysicalMonitor();
  if (!hMonitor)
    return -1;

  DWORD minBrightness = 0, currentBrightness = 0, maxBrightness = 0;
  bool success =
      GetMonitorBrightness(hMonitor, &minBrightness, &currentBrightness,
                           &maxBrightness) != 0;

  ClosePhysicalMonitor(hMonitor);
  return success ? static_cast<int>(currentBrightness) : -1;
}
```

- [ ] **Step 3: Build**

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/MonitorController.h src/MonitorController.cpp
git commit -m "feat: add SetBrightness/GetBrightness, refactor with OpenFirstPhysicalMonitor"
```

---

### Task 2: Create BrightnessItem class

**Files:**
- Create: `src/BrightnessItem.h`
- Create: `src/BrightnessItem.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write `src/BrightnessItem.h`**

```cpp
#pragma once

#include "PluginInterface.h"

class BrightnessItem : public IPluginItem {
public:
  const wchar_t *GetItemName() const override;
  const wchar_t *GetItemId() const override;
  const wchar_t *GetItemLableText() const override;
  const wchar_t *GetItemValueText() const override;
  const wchar_t *GetItemValueSampleText() const override;

  void UpdateBrightness(int value);
  void SetFormatOptions(bool noPercent, bool spaceBeforeUnit);

private:
  int  m_brightness = -1;
  bool m_noPercent = false;
  bool m_spaceBeforeUnit = false;
  wchar_t m_valueText[16] = L"";
};
```

- [ ] **Step 2: Write `src/BrightnessItem.cpp`**

```cpp
#include "BrightnessItem.h"
#include <cstdio>

const wchar_t *BrightnessItem::GetItemName() const { return L"亮度"; }

const wchar_t *BrightnessItem::GetItemId() const {
  return L"DDCCI_Brightness";
}

const wchar_t *BrightnessItem::GetItemLableText() const { return L"亮度"; }

const wchar_t *BrightnessItem::GetItemValueText() const {
  return m_valueText;
}

const wchar_t *BrightnessItem::GetItemValueSampleText() const {
  return L"100%";
}

void BrightnessItem::UpdateBrightness(int value) {
  m_brightness = value;
  if (value < 0) {
    m_valueText[0] = L'-';
    m_valueText[1] = L'-';
    m_valueText[2] = L'\0';
    return;
  }

  if (m_noPercent) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d", value);
  } else if (m_spaceBeforeUnit) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d %%", value);
  } else {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d%%", value);
  }
}

void BrightnessItem::SetFormatOptions(bool noPercent, bool spaceBeforeUnit) {
  m_noPercent = noPercent;
  m_spaceBeforeUnit = spaceBeforeUnit;
  UpdateBrightness(m_brightness);
}
```

- [ ] **Step 3: Update `CMakeLists.txt`** — add BrightnessItem.cpp to sources

```cmake
add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/BrightnessItem.cpp
    src/MonitorController.cpp
)
```

- [ ] **Step 4: Build**

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add src/BrightnessItem.h src/BrightnessItem.cpp CMakeLists.txt
git commit -m "feat: add BrightnessItem implementing IPluginItem for brightness display"
```

---

### Task 3: Update Plugin for brightness display and commands

**Files:**
- Modify: `src/Plugin.h`
- Modify: `src/Plugin.cpp`

- [ ] **Step 1: Update `src/Plugin.h`**

```cpp
#pragma once

#include "PluginInterface.h"
#include "BrightnessItem.h"

class Plugin : public ITMPlugin {
public:
  // ITMPlugin
  int          GetAPIVersion() const override;
  IPluginItem *GetItem(int index) override;
  void         DataRequired() override;
  const wchar_t *GetInfo(PluginInfoIndex index) override;
  int          GetCommandCount() override;
  const wchar_t *GetCommandName(int command_index) override;
  void         OnPluginCommand(int command_index, void *hWnd, void *para) override;
  void         OnInitialize(ITrafficMonitor *pApp) override;
  void         OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;

private:
  ITrafficMonitor *m_pApp = nullptr;
  BrightnessItem   m_brightnessItem;
  bool             m_noPercent = false;
  bool             m_spaceBeforeUnit = false;

  void RefreshBrightnessDisplay();
};
```

- [ ] **Step 2: Update `src/Plugin.cpp`**

```cpp
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
    return L"DDC/CI Brightness Control";
  case TMI_DESCRIPTION:
    return L"Control monitor brightness and power via DDC/CI";
  case TMI_AUTHOR:
  case TMI_COPYRIGHT:
  case TMI_URL:
    return L"";
  case TMI_VERSION:
    return L"1.1.0";
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
    if (MonitorController::SetBrightness(brightness)) {
      RefreshBrightnessDisplay();
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
```

- [ ] **Step 3: Build**

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/Plugin.h src/Plugin.cpp
git commit -m "feat: add brightness display item and 12 menu commands for brightness/power control"
```

---

### Task 4: Format, lint, and final build

**Files:**
- Format all source files
- Run clang-tidy
- Final build

- [ ] **Step 1: Format all source files**

```bash
clang-format -i src/dllmain.cpp src/Plugin.h src/Plugin.cpp src/BrightnessItem.h src/BrightnessItem.cpp src/MonitorController.h src/MonitorController.cpp
```

- [ ] **Step 2: Run clang-tidy**

```bash
clang-tidy src/dllmain.cpp src/Plugin.cpp src/BrightnessItem.cpp src/MonitorController.cpp -- -I include -I src -std=c++17 -DWIN32 -D_WINDOWS 2>&1 | grep -v "include\\\\"
```

Expected: zero warnings from project source files.

- [ ] **Step 3: Final build**

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected: build succeeds, `build/Release/DDCCIPlugin.dll` produced.

- [ ] **Step 4: Commit if any formatting/lint changes**

```bash
git add -u
git commit -m "chore: format and lint all source files"
```