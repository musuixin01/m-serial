# ESP32-S3 USB Serial/JTAG · Raw-Byte Echo Test

[回到 M_SERIAL](../../README.md) · [实际测试结果](../../docs/VALIDATION.md)

此示例提供独立 ESP-IDF 回显固件，以真实 ESP32-S3 原生 USB Serial/JTAG 接口作为 M_SERIAL 的测试对端。收到任意字节就原样回传，不添加换行、时间戳或模拟传感器值。

## 适用范围

- ESP32-S3，原生 USB Serial/JTAG 接口（通常 VID:PID 为 303A:1001）。
- ESP-IDF 6.1；有对应的 Xtensa 编译器和 Python 依赖。
- Python 3 + pyserial，电脑需能识别开发板的系统串口。
- 示例默认按**本次 16 MB Flash 开发板**配置。其他 ESP32-S3 板卡应在烧录前按硬件实际 Flash 大小调整配置，禁止盲目沿用。

这不是通用 STM32/ESP8266 固件，也不是 USB-UART 转换器的测试固件。

## 构建与烧录

在 ESP-IDF 环境已经激活的终端中运行：

~~~bash
cd examples/esp32s3-usb-echo
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
~~~

确认串口和目标硬件身份后，按需执行：

~~~bash
idf.py -p COM7 flash
~~~

**注意：烧录将覆盖开发板当前程序。** 请只用于你有权限修改的测试板。不要直接沿用 COM7；在其他电脑上应先确认实际端口。

## 字节一致性测试

确保已退出 M_SERIAL、串口监视器等可能占用该端口的程序：

~~~bash
python -m pip install pyserial
python test_usb_echo.py --port COM7
~~~

脚本会验证设备 USB VID/PID，并检查：

1. ASCII 文本。
2. UTF-8 中文文本。
3. 包含 00、FF 等字节的二进制数据。
4. 256 种字节取值 × 16，共 4096 字节。
5. 连续 8192 字节传输。
6. 空闲时不应出现额外数据。

本次真实设备测试以上项目全部通过，内容逐字节相等。该结果不等于所有 ESP32-S3 板卡或所有 USB 驱动环境均已验证。

## 调试

本项目此前还使用本机安装的 Xtensa GDB / Espressif OpenOCD 验证 USB-JTAG 的双核识别、app_main 硬件断点和单条指令执行。相关本机脚本涉及各机器不同的 SDK/驱动路径，未直接作为可移植工具发布；结果见 [验证记录](../../docs/VALIDATION.md)。

## 源码

- main/usb_echo_main.c：字节安全回显，不将输入当作 C 字符串。
- main/CMakeLists.txt：ESP-IDF USB Serial/JTAG 组件。
- sdkconfig.defaults：当前测试板默认配置。
- test_usb_echo.py：PC 端逐字节一致性验证工具。