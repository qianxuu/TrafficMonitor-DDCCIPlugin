#pragma once

#include <windows.h>

class MonitorController {
public:
  static bool TurnOff();
  static bool SetBrightness(int value);
  static int GetBrightness();

  // Set brightness and read back the actual value in one call
  // Returns the actual brightness, or -1 on failure
  static int SetAndGetBrightness(int value);

private:
  static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                       LPRECT lprcClip, LPARAM dwData);
  static HANDLE OpenFirstPhysicalMonitor();
  static void ClosePhysicalMonitor(HANDLE hMonitor);
};