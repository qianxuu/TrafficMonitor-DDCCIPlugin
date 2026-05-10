#pragma once

#include <windows.h>

class MonitorController {
public:
  static bool Standby();
  static bool TurnOff();
  static bool SetBrightness(int value);
  static int GetBrightness();

private:
  static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                       LPRECT lprcClip, LPARAM dwData);
  static HANDLE OpenFirstPhysicalMonitor();
};
