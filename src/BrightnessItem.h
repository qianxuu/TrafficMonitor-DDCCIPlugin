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