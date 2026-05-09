# DDC/CI 显示器亮度控制插件 — 设计文档

## 概述

为 TrafficMonitor 开发一个插件，通过 DDC/CI 协议控制显示器亮度和关闭显示器。插件提供一个显示项（显示当前亮度）和右键菜单命令（关闭显示器 + 亮度调节）。

## 显示项 (IPluginItem)

- **标签**：`亮度`
- **数值**：当前亮度值，格式适配主程序显示设置
- **显示方式**：文字显示，非自绘
- **刷新策略**：仅在加载时和用户操作亮度后更新缓存值，不主动轮询

### 亮度值格式

根据主程序通过 `OnExtenedInfo` 传递的设置决定：

| 配置 | `50%` | `50 %` | `50` |
|------|-------|--------|------|
| `EI_MAIN_WND_NOT_SHOW_PERCENT` | - | - | `"0"` |
| 默认（有百分号无空格） | `"50%"` | - | - |
| `EI_MAIN_WND_SPERATE_WITH_SPACE` +百分号 | - | `"50 %"` | - |

注意：主程序通过 `OnExtenedInfo` 传递的是主窗口设置（`EI_MAIN_WND_*`），插件统一使用这些值格式化显示。

## 右键菜单命令

共 12 条命令：

| 索引 | 命令名称 | 功能 | VCP |
|------|---------|------|-----|
| 0 | 关闭显示器 | DDC/CI 关闭显示器 | 0xD6=0x04 |
| 1 | 亮度 0 | 设置亮度为 0 | 0x10=0 |
| 2 | 亮度 10 | 设置亮度为 10 | 0x10=10 |
| 3 | 亮度 20 | 设置亮度为 20 | 0x10=20 |
| 4 | 亮度 30 | 设置亮度为 30 | 0x10=30 |
| 5 | 亮度 40 | 设置亮度为 40 | 0x10=40 |
| 6 | 亮度 50 | 设置亮度为 50 | 0x10=50 |
| 7 | 亮度 60 | 设置亮度为 60 | 0x10=60 |
| 8 | 亮度 70 | 设置亮度为 70 | 0x10=70 |
| 9 | 亮度 80 | 设置亮度为 80 | 0x10=80 |
| 10 | 亮度 90 | 设置亮度为 90 | 0x10=90 |
| 11 | 亮度 100 | 设置亮度为 100 | 0x10=100 |

## DDC/CI 操作

### API 选择

亮度调节使用 Windows 高级 Monitor Configuration API（`highlevelmonitorconfigurationapi.h`），比直接操作 VCP code 更简洁：

| 操作 | 函数 |
|------|------|
| 读取亮度 | `GetMonitorBrightness(hPhysicalMonitor, &min, &current, &max)` |
| 设置亮度 | `SetMonitorBrightness(hPhysicalMonitor, value)` |
| 关闭显示器 | `SetVCPFeature(hPhysicalMonitor, 0xD6, 0x04)`（无高级 API 对应） |

### VCP Codes

| VCP Code | 名称 | 范围 | 说明 |
|----------|------|------|------|
| 0x10 | Brightness | 0-100 | 屏幕亮度（通过高级 API 操作） |
| 0xD6 | Display Power Mode | 1/4 | 1=On, 4=Off |

### MonitorController 扩展

```cpp
class MonitorController {
public:
    static bool TurnOff();                          // 已有
    static bool SetBrightness(int value);           // 新增：设置亮度 0-100
    static int  GetBrightness();                    // 新增：读取当前亮度，失败返回 -1
};
```

## 架构调整

```
DDCCIPlugin/
├── include/
│   └── PluginInterface.h          (已有，不动)
├── src/
│   ├── dllmain.cpp                (不变)
│   ├── Plugin.def                 (不变)
│   ├── Plugin.h                   (修改：增加 BrightnessItem 成员)
│   ├── Plugin.cpp                 (修改：实现 IPluginItem、命令扩展)
│   ├── BrightnessItem.h           (新增：IPluginItem 实现)
│   ├── BrightnessItem.cpp         (新增)
│   ├── MonitorController.h        (修改：增加 SetBrightness/GetBrightness)
│   └── MonitorController.cpp      (修改：实现亮度读写)
├── CMakeLists.txt                 (修改：增加 BrightnessItem.cpp)
└── .clang-tidy
```

## 组件设计

### BrightnessItem (IPluginItem 实现)

- `GetItemName()` → `"亮度"`
- `GetItemId()` → `"DDCCI_Brightness"`
- `GetItemLableText()` → `"亮度"`
- `GetItemValueText()` → 缓存的亮度字符串（如 `"50%"` / `"50 %"` / `"50"`）
- `GetItemValueSampleText()` → `"100%"`（最长可能宽度）
- `UpdateBrightness(int value)` → 更新缓存值
- `SetFormatOptions(bool noPercent, bool spaceBeforeUnit)` → 设置格式选项

### Plugin (ITMPlugin 实现，修改)

- `GetItem(0)` → 返回 `BrightnessItem*`
- `GetCommandCount()` → 12
- `GetCommandName(0)` → `"关闭显示器"`
- `GetCommandName(1-11)` → `"亮度 0"` ~ `"亮度 100"`
- `OnPluginCommand(0)` → `MonitorController::TurnOff()`
- `OnPluginCommand(1-11)` → `MonitorController::SetBrightness(n)` + 更新显示
- `OnInitialize()` → 读取初始亮度 + 更新显示
- `OnExtenedInfo()` → 接收 `EI_MAIN_WND_NOT_SHOW_PERCENT` / `EI_MAIN_WND_SPERATE_WITH_SPACE`，传递给 BrightnessItem

## 错误处理

| 场景 | 处理 |
|------|------|
| 读取亮度失败 | 显示 `"--"` |
| 设置亮度失败 | 静默忽略 |
| 关闭显示器失败 | 静默忽略 |
| 操作成功 | 静默 |

## 构建

- CMake + MSVC (Visual Studio 2022)
- 输出：`DDCCIPlugin.dll` (x64)
- 依赖：`dxva2.lib`、`user32.lib`

## 不包含

- 多显示器选择
- 自动刷新亮度
- 自绘图形界面
- GUI 选项对话框
- 热键