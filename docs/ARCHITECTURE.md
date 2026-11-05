# M_SERIAL · Architecture

[回到首页](../README.md) · [测试记录](VALIDATION.md) · [路线图](ROADMAP.md)

## 当前架构：小而直接

M_SERIAL 目前是单窗口 Qt Widgets 工程；**尚未提取独立的 SerialCore 模块**。主要源码保留课程设计时期的实现，不把未来重构写成已经实现。

~~~mermaid
flowchart LR
    UI["Qt Widgets 主窗口<br/>参数设置 / 发送 / 接收"] <--> SERIAL["QSerialPort<br/>系统串口 API"]
    SERIAL <--> PORT["Windows COM / Linux tty"]
    PORT <--> MCU["开发板串口端点<br/>ESP32-S3 / STM32 等"]
    TIMER["QTimer<br/>自动发送"] --> UI
    UI --> LOG["QFile<br/>日志导出"]
~~~

## 源码分工

| 文件 | 职责 |
| --- | --- |
| main.cpp | Qt 应用程序入口 |
| mainwindow.ui | 窗口布局与控件层次 |
| mainwindow.h | 界面控制器状态与槽函数 |
| mainwindow.cpp | 串口发现、参数设置、收发、HEX/text 格式、自动发送、日志 |
| M_Serial.pro | qmake 配置：Qt Widgets、SerialPort、C++17 |

### 核心数据流

- 连接阶段通过 QSerialPortInfo 查询系统可见设备，用户选择具体串口与通讯参数。
- 文本发送使用 UTF-8 字节；HEX 模式从输入文本生成字节数组。
- QSerialPort::readyRead 触发读取并更新接收区。
- 自动发送使用 QTimer 复用发送路径。
- 接收区保存日志使用 Qt QFile，写入 UTF-8 文本。

### 测试对端

<code>examples/esp32s3-usb-echo</code> 是独立 ESP-IDF 固件；通过 ESP32-S3 **原生 USB Serial/JTAG** 收到字节就原样返回，既不转码也不重写结束符。

这与 M_SERIAL 在各种开发板上使用的串口通信模型一致，但示例固件本身仅针对 ESP32-S3 原生 USB Serial/JTAG。

## 已知技术边界

1. 目前界面、串口逻辑和日志操作集中于 MainWindow 类，未来可拆分为独立串口服务与解析层。
2. Qt 收到的每一块 QByteArray 当前分别执行 UTF-8 解码，跨块多字节字符可能出现乱码。
3. 目前 HEX 输入先移除非十六进制字符，可能掩盖输入错误。
4. 当前没有接收缓冲限制或系统性的错误信号处理。
5. 串口连接并不保证目标设备已正确配置固件、电气接口或数据协议。

修改这部分行为前应先加入回归测试，保护当前已验证的收发逻辑。