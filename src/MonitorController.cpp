#include "MonitorController.h"
#include <algorithm>
#include <highlevelmonitorconfigurationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <vector>
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

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *ctx = reinterpret_cast<EnumContext *>(dwData);
  ctx->hResult = hMonitor;
  return FALSE;
}

HANDLE MonitorController::OpenFirstPhysicalMonitor() {
  EnumContext ctx;
  HDC hdc = GetDC(nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const LPARAM contextParam = reinterpret_cast<LPARAM>(&ctx);
  EnumDisplayMonitors(hdc, nullptr, MonitorEnumProc, contextParam);
  ReleaseDC(nullptr, hdc);

  if (!ctx.hResult)
    return nullptr;

  DWORD cPhysicalMonitors = 0;
  if (!GetNumberOfPhysicalMonitorsFromHMONITOR(ctx.hResult,
                                               &cPhysicalMonitors) ||
      cPhysicalMonitors == 0)
    return nullptr;

  std::vector<PHYSICAL_MONITOR> physicalMonitors(cPhysicalMonitors);
  if (!GetPhysicalMonitorsFromHMONITOR(ctx.hResult, cPhysicalMonitors,
                                       physicalMonitors.data())) {
    return nullptr;
  }

  HANDLE hPhysicalMonitor = physicalMonitors[0].hPhysicalMonitor;
  for (DWORD i = 1; i < cPhysicalMonitors; ++i) {
    DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
  }
  return hPhysicalMonitor;
}

struct MonitorHandle {
  HANDLE h;
  MonitorHandle(HANDLE handle) : h(handle) {}
  MonitorHandle(const MonitorHandle &) = delete;
  MonitorHandle &operator=(const MonitorHandle &) = delete;
  ~MonitorHandle() {
    if (h) {
      DestroyPhysicalMonitor(h);
    }
  }
  operator HANDLE() const { return h; }
};

static int ReadBrightness(HANDLE hMonitor) {
  DWORD minBrightness = 0;
  DWORD currentBrightness = 0;
  DWORD maxBrightness = 0;
  bool success = GetMonitorBrightness(hMonitor, &minBrightness,
                                      &currentBrightness, &maxBrightness) != 0;

  return success ? static_cast<int>(currentBrightness) : -1;
}

bool MonitorController::Standby() {
  SendMessageW(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER,
               static_cast<LPARAM>(2));
  return true;
}

bool MonitorController::TurnOff() {
  MonitorHandle hMonitor(OpenFirstPhysicalMonitor());
  if (!hMonitor)
    return false;

  return SetVCPFeature(hMonitor, 0xD6, 0x05) != 0;
}

bool MonitorController::SetBrightness(int value) {
  MonitorHandle hMonitor(OpenFirstPhysicalMonitor());
  if (!hMonitor)
    return false;

  int v = std::clamp(value, 0, 100);
  return SetMonitorBrightness(hMonitor, static_cast<DWORD>(v)) != 0;
}

int MonitorController::GetBrightness() {
  MonitorHandle hMonitor(OpenFirstPhysicalMonitor());
  if (!hMonitor)
    return -1;

  return ReadBrightness(hMonitor);
}
