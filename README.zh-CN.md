# Bruce Scroller

**面向 LILYGO T-Embed CC1101 (ESP32-S3) 的 Bruce Firmware 蓝牙手机遥控器。** 转动旋钮发送鼠标滚轮事件，按键控制翻页和媒体。

[English](README.md) · [Русский](README.ru.md) · [简体中文](README.zh-CN.md)

Bruce Scroller 是基于 [Bruce Firmware](https://github.com/BruceDevices/firmware) 的小型 BLE HID 滚动功能修改版。它保留 Bruce 的菜单和现有功能，并为 **LILYGO T-Embed CC1101 (ESP32-S3)** 增加遥控模式。

## 功能

- 旋转编码器通过 Bluetooth LE 发送鼠标滚轮事件。
- 单击发送 Volume Down，双击发送 Volume Up。MangaLib 可将媒体键映射为上一页或下一页；具体行为取决于 Android 应用及其设置。
- 长按可循环切换五档滚动灵敏度。
- 连接后尝试将鼠标指针移离屏幕中央。指针显示由 Android 控制，因此不保证隐藏。
- 进入模式时绘制一次界面，随后只更新变化的状态行，避免原先周期性重绘整个屏幕。
- 保留 Bruce 中原有的 `Media Cmds` 等模式。

维护者报告已在一部 Android 手机上使用 MangaLib 测试。手机型号、Android 版本和应用版本没有记录为兼容性列表。TikTok、Instagram Reels、YouTube Shorts 和其他应用**尚未验证**；它们对滚轮和音量事件的处理可能不同。

## 兼容性

| 硬件 / 软件 | 状态 |
| --- | --- |
| [LILYGO T-Embed CC1101 (ESP32-S3)](https://github.com/Xinyuan-LilyGO/documentation/blob/master/en/products/t-embed-series/t-embed-cc1101/index.md) | 目标开发板；已验证此 PlatformIO 配置的构建 |
| 其他 Bruce 支持的开发板 | 本项目未测试，不声明兼容 |
| Android BLE HID 主机 | 设计为以 `Bruce Scroller` 名称配对；应用行为各异 |
| iOS、电脑或其他主机 | 未验证 |

本项目基于 Bruce Firmware commit [`2b6ba4d490bfcf94ce2574c6154537dbd5838c3d`](https://github.com/BruceDevices/firmware/commit/2b6ba4d490bfcf94ce2574c6154537dbd5838c3d)。Bruce 的其他功能请参阅[原始 README](README.bruce.md)和[项目 Wiki](https://github.com/BruceDevices/firmware/wiki)；第三方组件声明见 [`THIRD_PARTY.md`](THIRD_PARTY.md)。

## 操作

| 操作 | 功能 |
| --- | --- |
| 向右旋转 | 向下滚动 |
| 向左旋转 | 向上滚动 |
| 单击旋钮 | Volume Down；应用支持时翻到下一页 |
| 双击旋钮 | Volume Up；应用支持时翻到上一页 |
| 长按旋钮 | 在 1 到 5 档之间切换灵敏度 |
| 侧面 Back 按键 | 退出 Bruce Scroller 并返回 Bruce |

滚轮事件以 16 毫秒间隔逐步发送。有限队列可平滑短时间内的编码器输入；旋转过快时队列可能溢出，部分步进会丢失。连接后设备会发送少量相对鼠标移动，尝试将指针停靠在屏幕边缘；不保证指针隐藏。

## 构建

构建使用 Bruce PlatformIO 工程以及其中指定的平台和库版本。请安装 Git、Python 3 和 [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html)，然后运行：

```sh
git clone --recurse-submodules https://github.com/andreychatgptac-sketch/bruce-scroller.git
cd bruce-scroller
git submodule update --init --recursive
python -m pip install platformio
pio run -e lilygo-t-embed-cc1101
```

已验证的环境为 `lilygo-t-embed-cc1101`。该环境构建成功并不代表支持其他开发板。首次构建会下载工具链和库，可能需要一些时间。

GitHub Actions 会在相关源码变更和 pull request 上检查同一开发板的构建。该 workflow 不构建其他开发板，也不会发布固件 Release。

## 安装与使用

1. 刷写前备份设备上需要保留的数据。安装固件会替换当前固件。
2. 通过 USB 连接 **LILYGO T-Embed CC1101 (ESP32-S3)**，运行 `pio device list` 查看串口。
3. 为指定 PlatformIO 环境上传固件：

   ```sh
   pio run -e lilygo-t-embed-cc1101 -t upload --upload-port <PORT>
   ```

   将 `<PORT>` 替换为实际端口，例如 Windows 下的 `COM5` 或 Linux 下的 `/dev/ttyACM0`。上传工具会使用该开发板的分区和闪存设置；不要将应用镜像写入任意地址。
4. 在 Bruce 中打开 **Bluetooth → Bruce Scroller**。
5. 在手机蓝牙设置中与 **Bruce Scroller** 配对，然后在目标应用中测试操作。
6. 按侧面的 **Back** 按键退出并返回 Bruce。

如果 Android 保留了旧配对，请在蓝牙设置中删除旧的 Bruce HID 设备后重新配对。标准 BLE HID 不需要专门的 Android 应用，但不同应用的行为有所不同。

## Release 固件

发布合并固件时建议命名为 `Bruce-Scroller-<version>-LILYGO-T-Embed-CC1101-ESP32-S3.bin`，并在 Release 说明中注明源码 commit、SHA-256、构建环境和刷写步骤。本源码仓库不附带固件二进制文件；请按上述步骤本地构建。

## 许可证与致谢

Bruce Firmware 使用 **GNU AGPL v3.0 或更高版本**许可证；本仓库保留 Bruce 的许可证和署名文件。Bruce 还包含其他许可证的组件，详情见 [`THIRD_PARTY.md`](THIRD_PARTY.md)。Bruce Scroller 是社区独立修改版，并非 BruceDevices 或 LILYGO 官方产品。

## GitHub 仓库元数据

建议描述：**Bruce Firmware BLE HID phone remote for LILYGO T-Embed CC1101 (ESP32-S3): scroll and control media.**

建议 Topics：`bruce-firmware`、`lilygo-t-embed-cc1101`、`esp32-s3`、`ble-hid`、`bluetooth-le`、`android`、`phone-remote`、`media-remote`、`scroll-controller`、`manga-reader`、`social-media`、`firmware`。
