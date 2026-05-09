# DDC/CI 显示器关闭插件 — 设计文档

## 概述

为 TrafficMonitor 开发一个插件，通过右键菜单命令，使用 DDC/CI 协议关闭显示器。

## 触发方式

- **右键菜单命令**：插件注册一条命令"关闭显示器"，用户右键 TrafficMonitor 图标 → 点击命令 → 关闭显示器。
- 无显示项目（IPluginItem），不需要在界面上显示任何监控数据。

## 显示器范围

- 仅操作主显示器（通过 `MonitorFromPoint` 或枚举第一个物理显示器）。

## 功能

- 仅关闭（发送 DDC/CI VCP Code 0xD6 = 0x04 断电模式）。
- 唤醒通过物理按键/鼠标移动。
- 关闭成功后不弹出通知。

## DDC/CI 实现

- 使用 Windows 原生 Monitor Configuration API（`dxva2.dll`）。
- 不需要任何第三方库。

### 关键 API

| API | 用途 |
|-----|------|
| `EnumDisplayMonitors` | 枚举显示器，获取第一个物理 HMONITOR |
| `GetPhysicalMonitorsFromHMONITOR` | 获取物理显示器句柄 |
| `GetVCPFeatureAndVCPFeatureReply` | 读取 VCP 功能（可选，用于检测 DDC/CI 支持） |
| `SetVCPFeature` | 设置 VCP 功能（关闭显示器：code=0xD6, value=0x04） |
| `DestroyPhysicalMonitors` | 释放物理显示器句柄 |

### VCP Code 0xD6 (Display Power Mode)

| 值 | 含义 |
|----|------|
| 0x01 | DPM: On（开启） |
| 0x04 | DPM: Off（关闭背光） |

## 架构

```
DDCCIPlugin/
├── include/
│   └── PluginInterface.h      (已有)
├── src/
│   ├── Plugin.h               — ITMPlugin 实现
│   ├── Plugin.cpp             — 实现所有 ITMPlugin 虚函数
│   └── MonitorController.h    — DDC/CI 封装
│   └── MonitorController.cpp  — 调用 Windows Monitor Config API
├── CMakeLists.txt             — 构建配置 (CMake)
├── .gitignore
└── docs/
    └── superpowers/
        └── specs/
            └── 2026-05-09-ddcci-monitor-off-design.md
```

## 组件设计

### Plugin（ITMPlugin 实现）

- `GetAPIVersion()` → 7
- `GetItem(int index)` → nullptr（无显示项目）
- `DataRequired()` → 空实现
- `GetInfo(index)` → 返回插件名称、描述、作者、版本等信息
- `GetCommandCount()` → 1
- `GetCommandName(0)` → "关闭显示器"
- `OnPluginCommand(0, ...)` → 调用 `MonitorController::TurnOff()`，静默执行
- `OnInitialize(pApp)` → 保存 `ITrafficMonitor*` 指针

### MonitorController（DDC/CI 封装）

- `static bool TurnOff()` — 关闭主显示器
  - 枚举显示器，取第一个物理显示器
  - 调用 `SetVCPFeature(hPhysicalMonitor, 0xD6, 0x04)`
  - 返回是否成功
- `static bool IsDDCCISupported(HMONITOR hMonitor)` — 检查 DDC/CI 支持（可选）

## 错误处理

| 场景 | 处理 |
|------|------|
| 无显示器连接 | 弹出通知"未找到显示器" |
| 显示器不支持 DDC/CI | 弹出通知"显示器不支持 DDC/CI" |
| SetVCPFeature 失败 | 弹出通知"关闭显示器失败" |
| 关闭成功 | 静默，不弹通知 |

## 构建

- 使用 CMake + MSVC（Visual Studio）
- 输出：`DDCCIPlugin.dll`（x64）
- 依赖：`dxva2.lib`、`user32.lib`

## 不包含

- 多显示器选择
- 软件开启显示器
- 定时关闭
- 热键触发
- GUI 选项对话框