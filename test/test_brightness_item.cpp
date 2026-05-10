#include "BrightnessItem.h"
#include <cassert>
#include <cstdio>
#include <cwchar>

static void Test_DefaultValue_Is_N_A() {
  BrightnessItem item;
  assert(std::wcscmp(item.GetItemValueText(), L"N/A") == 0);
}

static void Test_DefaultSampleText_Is_100percent() {
  BrightnessItem item;
  assert(std::wcscmp(item.GetItemValueSampleText(), L"100%") == 0);
}

static void Test_UpdateBrightness_DefaultFormat() {
  BrightnessItem item;
  item.UpdateBrightness(50);
  assert(std::wcscmp(item.GetItemValueText(), L"50%") == 0);
}

static void Test_NegativeValue_ShowsN_A() {
  BrightnessItem item;
  item.UpdateBrightness(75);
  assert(std::wcscmp(item.GetItemValueText(), L"75%") == 0);
  item.UpdateBrightness(-1);
  assert(std::wcscmp(item.GetItemValueText(), L"N/A") == 0);
}

static void Test_NoPercent_Format() {
  BrightnessItem item;
  item.SetFormatOptions(true, false);
  item.UpdateBrightness(50);
  assert(std::wcscmp(item.GetItemValueText(), L"50") == 0);
}

static void Test_SpaceBeforeUnit_Format() {
  BrightnessItem item;
  item.SetFormatOptions(false, true);
  item.UpdateBrightness(75);
  assert(std::wcscmp(item.GetItemValueText(), L"75 %") == 0);
}

static void Test_DefaultPlusSpace_Format() {
  BrightnessItem item;
  item.SetFormatOptions(false, false);
  item.UpdateBrightness(30);
  assert(std::wcscmp(item.GetItemValueText(), L"30%") == 0);
}

static void Test_SampleText_NoPercent() {
  BrightnessItem item;
  item.SetFormatOptions(true, false);
  assert(std::wcscmp(item.GetItemValueSampleText(), L"100") == 0);
}

static void Test_SampleText_SpaceBeforeUnit() {
  BrightnessItem item;
  item.SetFormatOptions(false, true);
  assert(std::wcscmp(item.GetItemValueSampleText(), L"100 %") == 0);
}

static void Test_SampleText_Default() {
  BrightnessItem item;
  item.SetFormatOptions(false, false);
  assert(std::wcscmp(item.GetItemValueSampleText(), L"100%") == 0);
}

static void Test_FormatChange_PreservesValue() {
  BrightnessItem item;
  item.UpdateBrightness(60);
  item.SetFormatOptions(true, false);
  assert(std::wcscmp(item.GetItemValueText(), L"60") == 0);
  item.SetFormatOptions(false, true);
  assert(std::wcscmp(item.GetItemValueText(), L"60 %") == 0);
  item.SetFormatOptions(false, false);
  assert(std::wcscmp(item.GetItemValueText(), L"60%") == 0);
}

static void Test_ZeroBrightness() {
  BrightnessItem item;
  item.UpdateBrightness(0);
  assert(std::wcscmp(item.GetItemValueText(), L"0%") == 0);
}

static void Test_MaxBrightness_100() {
  BrightnessItem item;
  item.UpdateBrightness(100);
  assert(std::wcscmp(item.GetItemValueText(), L"100%") == 0);
}

static void Test_MaxBrightness_100_NoPercent() {
  BrightnessItem item;
  item.SetFormatOptions(true, false);
  item.UpdateBrightness(100);
  assert(std::wcscmp(item.GetItemValueText(), L"100") == 0);
}

static void Test_MaxBrightness_100_Space() {
  BrightnessItem item;
  item.SetFormatOptions(false, true);
  item.UpdateBrightness(100);
  assert(std::wcscmp(item.GetItemValueText(), L"100 %") == 0);
}

static void Test_BufferOverflow_100Percent() {
  BrightnessItem item;
  item.UpdateBrightness(100);
  assert(std::wcscmp(item.GetItemValueText(), L"100%") == 0);
}

static void Test_BufferOverflow_100Space() {
  BrightnessItem item;
  item.SetFormatOptions(false, true);
  item.UpdateBrightness(100);
  assert(std::wcscmp(item.GetItemValueText(), L"100 %") == 0);
}

static void Test_ItemName_And_Id() {
  BrightnessItem item;
  assert(std::wcscmp(item.GetItemName(), L"显示器亮度") == 0);
  assert(std::wcscmp(item.GetItemId(), L"DDCCIBrightness") == 0);
  assert(std::wcscmp(item.GetItemLableText(), L"亮度: ") == 0);
}

int main() {
  Test_DefaultValue_Is_N_A();
  Test_DefaultSampleText_Is_100percent();
  Test_UpdateBrightness_DefaultFormat();
  Test_NegativeValue_ShowsN_A();
  Test_NoPercent_Format();
  Test_SpaceBeforeUnit_Format();
  Test_DefaultPlusSpace_Format();
  Test_SampleText_NoPercent();
  Test_SampleText_SpaceBeforeUnit();
  Test_SampleText_Default();
  Test_FormatChange_PreservesValue();
  Test_ZeroBrightness();
  Test_MaxBrightness_100();
  Test_MaxBrightness_100_NoPercent();
  Test_MaxBrightness_100_Space();
  Test_BufferOverflow_100Percent();
  Test_BufferOverflow_100Space();
  Test_ItemName_And_Id();

  std::printf("All BrightnessItem tests passed.\n");
  return 0;
}
