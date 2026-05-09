#include "BrightnessItem.h"
#include <cstdio>

const wchar_t *BrightnessItem::GetItemName() const { return L"亮度"; }

const wchar_t *BrightnessItem::GetItemId() const { return L"DDCCI_Brightness"; }

const wchar_t *BrightnessItem::GetItemLableText() const { return L"亮度"; }

const wchar_t *BrightnessItem::GetItemValueText() const { return m_valueText; }

const wchar_t *BrightnessItem::GetItemValueSampleText() const {
  return L"100%";
}

void BrightnessItem::UpdateBrightness(int value) {
  m_brightness = value;
  if (value < 0) {
    m_valueText[0] = L'-';
    m_valueText[1] = L'-';
    m_valueText[2] = L'\0';
    return;
  }

  if (m_noPercent) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d", value);
  } else if (m_spaceBeforeUnit) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d %%", value);
  } else {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d%%", value);
  }
}

void BrightnessItem::SetFormatOptions(bool noPercent, bool spaceBeforeUnit) {
  m_noPercent = noPercent;
  m_spaceBeforeUnit = spaceBeforeUnit;
  UpdateBrightness(m_brightness);
}