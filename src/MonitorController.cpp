#include "MonitorController.h"
#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>

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
    return FALSE;
}

bool MonitorController::TurnOff()
{
    EnumContext ctx;
    HDC hdc = GetDC(nullptr);
    EnumDisplayMonitors(hdc, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&ctx));
    ReleaseDC(nullptr, hdc);

    if (!ctx.hResult)
        return false;

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

    HANDLE hPhysicalMonitor = pPhysicalMonitors[0].hPhysicalMonitor;

    if (SetVCPFeature(hPhysicalMonitor, 0xD6, 0x04))
    {
        success = true;
    }

    DestroyPhysicalMonitors(cPhysicalMonitors, pPhysicalMonitors);
    delete[] pPhysicalMonitors;

    return success;
}