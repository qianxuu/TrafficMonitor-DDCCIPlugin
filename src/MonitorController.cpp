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

  DWORD minBrightness = 0;
  DWORD currentBrightness = 0;
  DWORD maxBrightness = 0;
  bool success = GetMonitorBrightness(hMonitor, &minBrightness,
                                      &currentBrightness, &maxBrightness) != 0;

  ClosePhysicalMonitor(hMonitor);
  return success ? static_cast<int>(currentBrightness) : -1;
}

int MonitorController::SetAndGetBrightness(int value) {
  HANDLE hMonitor = OpenFirstPhysicalMonitor();
  if (!hMonitor)
    return -1;

  if (SetMonitorBrightness(hMonitor, static_cast<DWORD>(value)) == 0) {
    ClosePhysicalMonitor(hMonitor);
    return -1;
  }

  DWORD minBrightness = 0;
  DWORD currentBrightness = 0;
  DWORD maxBrightness = 0;
  int result = -1;
  if (GetMonitorBrightness(hMonitor, &minBrightness, &currentBrightness,
                           &maxBrightness) != 0) {
    result = static_cast<int>(currentBrightness);
  }

  ClosePhysicalMonitor(hMonitor);
  return result;
}