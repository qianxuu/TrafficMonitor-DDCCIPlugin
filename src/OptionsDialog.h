#pragma once

#include <string>
#include <windows.h>

struct OptionsDialogResult {
  bool accepted = false;
  std::wstring presetText;
};

OptionsDialogResult
ShowBrightnessPresetOptionsDialog(HWND parent,
                                  const std::wstring &currentPresetText);
