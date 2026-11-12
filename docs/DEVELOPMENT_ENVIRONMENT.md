# M_SERIAL · 开发与调试环境

[首页](../README.md) · [架构](ARCHITECTURE.md) · [验证](VALIDATION.md)

## Qt 工程

- 语言及框架：C++17、Qt 6 Widgets、Qt SerialPort。
- IDE 与构建：Qt Creator、qmake 工程（M_Serial.pro）。
- 已验证的 Windows 环境：Qt 6.11.2、MinGW 13.1。

## RK3568 / Embedded Linux

RK3568 目标板开发涉及 Buildroot 系统、ARM GCC/G++ 交叉编译、Qt 库与板卡 SDK。交叉编译器、sysroot、ABI、Qt 库必须互相匹配，不能仅凭 RK3568 SoC 型号断定工具链位数。参考手册中的 arm-buildroot-linux-gnueabihf 是示例前缀，不代表每块开发板的实际 SDK。

项目此前在 RK3568 开发板上启动运行过，但尚未完成该平台完整的串口功能回归。

## MobaXterm：Serial 与 SSH

- **Serial 串口终端**：连接 RK3568 开发板串口控制台，查看启动输出并使用控制台。
- **SSH 远程终端**：通过网络登录 RK3568 Linux，访问远程 Shell、管理目标程序。

两种连接方式均用于开发板调试。MobaXterm 不属于 M_SERIAL 的运行时依赖，也不是 Qt SerialPort 的实现方式。

## ADB

ADB 用作独立的设备连接与管理通道。实际连接方式取决于设备系统配置；它与 M_SERIAL 使用 COM/tty 进行串口数据收发的通道相互独立。

## ESP32-S3 硬件验证与 GDB

M_SERIAL 使用 ESP-IDF 构建 ESP32-S3 USB Serial/JTAG 字节回显固件作为测试对端。当前仓库记录了 Windows 端真实串口通信、4096/4096 与 8192/8192 字节一致性测试，以及 GDB/OpenOCD 硬件断点与单步调试。

这些结果只覆盖所述测试环境。RK3568 尚未重新完成完整功能测试，Linux x64 与 macOS 也未开展本轮独立验证。
