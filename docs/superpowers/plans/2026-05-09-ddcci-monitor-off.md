# DDC/CI 显示器关闭插件 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a TrafficMonitor plugin DLL that adds a right-click menu command to turn off the monitor via DDC/CI protocol.

**Architecture:** A minimal COM-free DLL exporting `TMPluginGetInstance()`, implementing `ITMPlugin` with one menu command. DDC/CI operations are encapsulated in a static `MonitorController` class calling Windows Monitor Configuration API (`dxva2.dll`).

**Tech Stack:** C++17, Windows SDK, MSVC (VS2022), CMake

---

## File Layout

```
DDCCIPlugin/
├── include/
│   └── PluginInterface.h           (existing — read only)
├── src/
│   ├── Plugin.h                    (create — ITMPlugin impl declaration)
│   ├── Plugin.cpp                  (create — ITMPlugin impl definition)
│   └── MonitorController.h         (create — DDC/CI wrapper declaration)
│   └── MonitorController.cpp       (create — DDC/CI wrapper definition)
│   └── dllmain.cpp                 (create — DllMain + TMPluginGetInstance)
│   └── Plugin.def                  (create — exports)
├── CMakeLists.txt                  (create)
└── .gitignore                      (create)
```

---

### Task 1: Project scaffolding

**Files:**
- Create: `src/dllmain.cpp`
- Create: `src/Plugin.def`
- Create: `CMakeLists.txt`
- Create: `.gitignore`

- [ ] **Step 1: Write `.gitignore`**

```
build/
.vs/
*.user
*.dll
*.exp
*.lib
*.pdb
x64/
```

- [ ] **Step 2: Write `src/dllmain.cpp`** — DllMain + exported factory

```cpp
#include <windows.h>
#include "Plugin.h"

static Plugin g_plugin;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        // cleanup if needed
    }
    return TRUE;
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &g_plugin;
}
```

- [ ] **Step 3: Write `src/Plugin.def`**

```
EXPORTS
    TMPluginGetInstance
```

- [ ] **Step 4: Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(DDCCIPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(DDCCIPlugin SHARED
    src/dllmain.cpp
    src/Plugin.cpp
    src/MonitorController.cpp
)

target_include_directories(DDCCIPlugin PRIVATE
    include
    src
)

target_link_libraries(DDCCIPlugin PRIVATE
    dxva2
    user32
)

set_target_properties(DDCCIPlugin PROPERTIES
    LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/Plugin.def"
)
```

- [ ] **Step 5: Build to verify scaffolding compiles**

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected: build fails with "Plugin.h not found" — that's Task 2.

- [ ] **Step 6: Commit**

```bash
git add .gitignore CMakeLists.txt src/dllmain.cpp src/Plugin.def
git commit -m "chore: add project scaffolding, CMakeLists, DllMain, exports"
```

---

### Task 2: Plugin class — empty shell

**Files:**
- Create: `src/Plugin.h`
- Create: `src/Plugin.cpp`

- [ ] **Step 1: Write `src/Plugin.h`**

```cpp
#pragma once

#include "PluginInterface.h"

class Plugin : public ITMPlugin
{
public:
    // ITMPlugin
    int          GetAPIVersion() const override;
    IPluginItem* GetItem(int index) override;
    void         DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    int          GetCommandCount() override;
    const wchar_t* GetCommandName(int command_index) override;
    void         OnPluginCommand(int command_index, void* hWnd, void* para) override;
    void         OnInitialize(ITrafficMonitor* pApp) override;

private:
    ITrafficMonitor* m_pApp = nullptr;
};
```

- [ ] **Step 2: Write `src/Plugin.cpp`** — all methods return safe defaults, no real logic yet

```cpp
#include "Plugin.h"

int Plugin::GetAPIVersion() const
{
    return 7;
}

IPluginItem* Plugin::GetItem(int index)
{
    (void)index;
    return nullptr;
}

void Plugin::DataRequired()
{
}

const wchar_t* Plugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:        return L"DDC/CI Monitor Off";
    case TMI_DESCRIPTION: return L"Turn off the monitor via DDC/CI protocol";
    case TMI_AUTHOR:      return L"";  // fill in as desired
    case TMI_COPYRIGHT:   return L"";
    case TMI_VERSION:     return L"1.0.0";
    case TMI_URL:         return L"";
    default:              return L"";
    }
}

int Plugin::GetCommandCount()
{
    return 1;
}

const wchar_t* Plugin::GetCommandName(int command_index)
{
    if (command_index == 0)
        return L"Turn off monitor";
    return nullptr;
}

void Plugin::OnPluginCommand(int command_index, void* hWnd, void* para)
{
    (void)command_index;
    (void)hWnd;
    (void)para;
    // Will be implemented in Task 4
}

void Plugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_pApp = pApp;
}
```

- [ ] **Step 3: Build**

```bash
cmake --build build --config Release
```

Expected: build succeeds (MonitorController.cpp will be compiled later).

- [ ] **Step 4: Commit**

```bash
git add src/Plugin.h src/Plugin.cpp
git commit -m "feat: add Plugin class shell implementing ITMPlugin"
```

---

### Task 3: MonitorController — DDC/CI wrapper

**Files:**
- Create: `src/MonitorController.h`
- Create: `src/MonitorController.cpp`

- [ ] **Step 1: Write `src/MonitorController.h`**

```cpp
#pragma once

class MonitorController
{
public:
    // Turn off the primary monitor. Returns true on success.
    static bool TurnOff();

private:
    // Callback for EnumDisplayMonitors — captures the first HMONITOR found
    static BOOL CALLBACK MonitorEnumProc(
        HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcClip, LPARAM dwData);
};
```

- [ ] **Step 2: Write `src/MonitorController.cpp`**

```cpp
#include "MonitorController.h"
#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>

#pragma comment(lib, "dxva2.lib")

struct EnumContext
{
    HMONITOR hResult = nullptr;
};

BOOL CALLBACK MonitorController::MonitorEnumProc(
    HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcClip, LPARAM dwData)
{
    (void)hdcMonitor;
    (void)lprcClip;

    auto* ctx = reinterpret_cast<EnumContext*>(dwData);
    ctx->hResult = hMonitor;
    return FALSE; // Stop enumeration after first match
}

bool MonitorController::TurnOff()
{
    // Enumerate display monitors, grab the first one
    EnumContext ctx;
    HDC hdc = GetDC(nullptr);
    EnumDisplayMonitors(hdc, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&ctx));
    ReleaseDC(nullptr, hdc);

    if (!ctx.hResult)
        return false;

    // Get physical monitor handle
    DWORD cPhysicalMonitors = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(ctx.hResult, &cPhysicalMonitors) || cPhysicalMonitors == 0)
        return false;

    PHYSICAL_MONITOR* pPhysicalMonitors = new PHYSICAL_MONITOR[cPhysicalMonitors];
    if (!GetPhysicalMonitorsFromHMONITOR(ctx.hResult, cPhysicalMonitors, pPhysicalMonitors))
    {
        delete[] pPhysicalMonitors;
        return false;
    }

    bool success = false;

    // Use the first physical monitor
    HANDLE hPhysicalMonitor = pPhysicalMonitors[0].hPhysicalMonitor;

    // VCP Code 0xD6 (Display Power Mode), value 0x04 (DPM: Off)
    if (SetVCPFeature(hPhysicalMonitor, 0xD6, 0x04))
    {
        success = true;
    }

    // Cleanup
    DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
    delete[] pPhysicalMonitors;

    return success;
}
```

- [ ] **Step 3: Build**

```bash
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 4: Quick verification — check that exports look right**

```bash
dumpbin /EXPORTS build/Release/DDCCIPlugin.dll
```

Expected: `TMPluginGetInstance` appears in exports table.

- [ ] **Step 5: Commit**

```bash
git add src/MonitorController.h src/MonitorController.cpp
git commit -m "feat: add MonitorController with DDC/CI TurnOff via Windows Monitor Config API"
```

---

### Task 4: Wire command to MonitorController

**Files:**
- Modify: `src/Plugin.cpp`

- [ ] **Step 1: Include MonitorController at top of `Plugin.cpp`**

```cpp
#include "Plugin.h"
#include "MonitorController.h"
```

Replace the current single `#include "Plugin.h"` line with the above.

- [ ] **Step 2: Implement `OnPluginCommand`**

Replace the stub implementation:

```cpp
void Plugin::OnPluginCommand(int command_index, void* hWnd, void* para)
{
    (void)command_index;
    (void)hWnd;
    (void)para;
    // Will be implemented in Task 4
}
```

With:

```cpp
void Plugin::OnPluginCommand(int command_index, void* hWnd, void* para)
{
    (void)hWnd;
    (void)para;

    if (command_index == 0)
    {
        MonitorController::TurnOff();
    }
}
```

- [ ] **Step 3: Build**

```bash
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/Plugin.cpp
git commit -m "feat: wire menu command to MonitorController::TurnOff"
```