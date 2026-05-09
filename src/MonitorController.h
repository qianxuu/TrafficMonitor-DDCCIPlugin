#pragma once

#include <windows.h>

class MonitorController {
public:
  static bool TurnOff();

private:
  static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                       LPRECT lprcClip, LPARAM dwData);
};