#include "BrightnessItem.h"
#include <cstdio>
#include <cwchar>

const wchar_t *BrightnessItem::GetItemName() const { return L"显示器亮度"; }

const wchar_t *BrightnessItem::GetItemId() const { return L"DDCCIBrightness"; }

const wchar_t *BrightnessItem::GetItemLableText() const { return L"亮度: "; }

const wchar_t *BrightnessItem::GetItemValueText() const { return m_valueText; }

const wchar_t *BrightnessItem::GetItemValueSampleText() const {
  if (m_noPercent)
    return L"100";
  if (m_spaceBeforeUnit)
    return L"100 %";
  return L"100%";
}

void BrightnessItem::UpdateBrightness(int value) {
  m_brightness = value;
  FormatValueText();
}

void BrightnessItem::SetFormatOptions(bool noPercent, bool spaceBeforeUnit) {
  m_noPercent = noPercent;
  m_spaceBeforeUnit = spaceBeforeUnit;
  FormatValueText();
}

void BrightnessItem::FormatValueText() {
  if (m_brightness < 0) {
    wcscpy_s(m_valueText, L"N/A");
    return;
  }

  if (m_noPercent) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d", m_brightness);
  } else if (m_spaceBeforeUnit) {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d %%", m_brightness);
  } else {
    _snwprintf_s(m_valueText, 16, _TRUNCATE, L"%d%%", m_brightness);
  }
}
