# DDCCIPlugin

DDCCIPlugin 是一个用于 [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) 的显示器控制插件。它通过 DDC/CI 协议读取和控制显示器亮度，并提供常用亮度预设和显示器电源命令。

## 功能特性

- 在 TrafficMonitor 中显示当前显示器亮度
- 通过插件命令快速设置亮度
- 支持用户自定义亮度预设列表
- 支持显示器待机和关机命令
- 亮度显示格式跟随 TrafficMonitor 的百分号和空格设置

## 使用要求

- Windows
- TrafficMonitor
- 支持 DDC/CI 的显示器
- 支持 DDC/CI 控制的连接方式和显卡驱动

如果显示器、线缆、扩展坞或显卡驱动不支持 DDC/CI，插件可能无法读取或设置亮度。

## 安装

1. 获取或自行构建 `DDCCIPlugin.dll`。
2. 将 `DDCCIPlugin.dll` 放入 TrafficMonitor 的插件目录。
3. 在 TrafficMonitor 中启用插件。
4. 在 TrafficMonitor 的显示项目设置中添加“显示器亮度”。

## 使用说明

启用插件后，可以在 TrafficMonitor 中查看当前亮度。通过插件命令菜单可以选择亮度预设，也可以执行显示器待机或关机命令。

在插件选项中可以配置亮度预设列表。预设值使用空格分隔，例如：

```text
0 25 50 75 100
```

配置规则：

- 每个值必须是 `0` 到 `100` 之间的整数
- 重复值会按首次出现保留
- 空配置或任一非法值会让亮度预设列表失效，此时只显示电源命令

## 配置文件

插件会在 TrafficMonitor 提供的插件配置目录中保存配置文件：

```ini
DDCCIPlugin.ini
```

当前配置项：

```ini
BrightnessPresets=0 25 50 75 100
```

首次运行且配置文件不存在时，插件会自动创建配置文件并写入默认预设。

## 从源码构建

依赖：

- CMake 3.20 或更高版本
- Visual Studio 2022 Build Tools 或完整 Visual Studio
- Windows SDK

构建：

```bash
cmake -S . -B build
cmake --build build
```

运行测试：

```bash
ctest --test-dir build -C Debug --output-on-failure
```

构建产物位于 CMake 生成的构建目录中，例如：

```text
build/Debug/DDCCIPlugin.dll
```

## 已知限制

- 当前只控制枚举到的第一个物理显示器
- 部分显示器或连接方式可能不支持 DDC/CI 亮度控制
- 显示器待机/关机命令的实际行为取决于显示器固件实现
- 插件不会绕过系统或显示器自身的亮度控制限制

## 版本

当前版本：`1.2.0`

## License

待定。
