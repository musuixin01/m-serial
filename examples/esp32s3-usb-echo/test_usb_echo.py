import argparse
import time

import serial
from serial.tools import list_ports


def read_exact(port, size, timeout=8.0):
    deadline = time.monotonic() + timeout
    received = bytearray()
    while len(received) < size and time.monotonic() < deadline:
        chunk = port.read(size - len(received))
        if chunk:
            received.extend(chunk)
    return bytes(received)


def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 USB Serial/JTAG raw-byte echo test")
    parser.add_argument("--port", default="COM7", help="Expected native USB Serial/JTAG port (default: COM7)")
    args = parser.parse_args()

    devices = {p.device: p for p in list_ports.comports()}
    device = devices.get(args.port)
    if device is None:
        raise SystemExit(f"FAIL: {args.port} is not present")
    if device.vid != 0x303A or device.pid != 0x1001:
        raise SystemExit(
            f"FAIL: {args.port} is not Espressif USB Serial/JTAG "
            f"(VID:PID={device.vid!r}:{device.pid!r})"
        )

    cases = [
        ("ASCII", b"M_Serial USB Serial/JTAG ECHO\r\n"),
        ("UTF-8", "串口助手中文收发测试".encode("utf-8")),
        ("Binary including NUL/0xFF", bytes([0x00, 0x01, 0x02, 0x7F, 0x80, 0xFE, 0xFF, 0x0D, 0x0A, 0x00])),
        ("All byte values x16", bytes(range(256)) * 16),
        ("Continuous 8192 bytes", bytes((i * 73 + 19) & 0xFF for i in range(8192))),
    ]

    print(f"Device: {device.device} ({device.description})")
    port = serial.Serial(port=None, baudrate=115200, timeout=0.15, write_timeout=3)
    port.port = args.port
    port.dtr = False
    port.rts = False
    port.open()
    with port:
        time.sleep(0.5)
        port.reset_input_buffer()

        for name, payload in cases:
            port.write(payload)
            port.flush()
            received = read_exact(port, len(payload))
            passed = received == payload
            print(
                f"{'PASS' if passed else 'FAIL'} {name}: "
                f"sent={len(payload)} received={len(received)} exact_match={passed}"
            )
            if not passed:
                print(f"expected[0:24]={payload[:24].hex()}")
                print(f"received[0:24]={received[:24].hex()}")
                raise SystemExit(2)
            time.sleep(0.05)

        time.sleep(0.25)
        extra = port.read(64)
        print(f"Idle extra bytes: {len(extra)}")
        if extra:
            raise SystemExit("FAIL: unexpected data while idle")

    print("ALL ESP32-S3 USB ECHO TESTS PASSED")


if __name__ == "__main__":
    main()
