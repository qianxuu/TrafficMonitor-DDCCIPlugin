#include "BrightnessItem.h"
#include <cstdio>

const wchar_t *BrightnessItem::GetItemName() const { return L"显示器亮度"; }

const wchar_t *BrightnessItem::GetItemId() const { return L"DDCCIBrightness"; }

const wchar_t *BrightnessItem::GetItemLableText() const { return L"亮度: "; }

const wchar_t *BrightnessItem::GetItemValueText() const {
  if (m_brightness < 0) {
    static const wchar_t s_dash[] = L"--";
    return s_dash;
  }

  static wchar_t s_buf[16];
  if (m_noPercent) {
    _snwprintf_s(s_buf, 16, _TRUNCATE, L"%d", m_brightness);
  } else if (m_spaceBeforeUnit) {
    _snwprintf_s(s_buf, 16, _TRUNCATE, L"%d %%", m_brightness);
  } else {
    _snwprintf_s(s_buf, 16, _TRUNCATE, L"%d%%", m_brightness);
  }
  return s_buf;
}

const wchar_t *BrightnessItem::GetItemValueSampleText() const {
  return GetFormattedSampleText();
}

void BrightnessItem::UpdateBrightness(int value) {
  m_brightness = value;
}

void BrightnessItem::SetFormatOptions(bool noPercent, bool spaceBeforeUnit) {
  m_noPercent = noPercent;
  m_spaceBeforeUnit = spaceBeforeUnit;
}

const wchar_t *BrightnessItem::GetFormattedSampleText() const {
  if (m_noPercent)
    return L"100";
  if (m_spaceBeforeUnit)
    return L"100 %";
  return L"100%";
}