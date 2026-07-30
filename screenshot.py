#!/usr/bin/env python3
"""
ESP32-S3-RLCD-4.2 Screen Capture Tool

Usage:
    python screenshot.py [COM_PORT] [output.png]

Modes (select via flag):
    --reset   Reset device and wait for auto-screenshot after WiFi connects (default).
    --listen  Listen without reset; press BOOT button to trigger.
    --shoot   Send serial SHOOT command (may not work on USB-Serial/JTAG).

Defaults:
    COM_PORT = COM4
    output   = screenshot_<timestamp>.png  (or .pbm if Pillow not installed)
"""

import sys
import time
import base64
from datetime import datetime

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

# Optional: Pillow for PBM -> PNG conversion
try:
    from PIL import Image
    import io
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def receive_screenshot(ser, timeout=20):
    """Listen for SCREENSHOT_START/END markers, return decoded PBM bytes."""
    collecting = False
    b64_chunks = []
    t0 = time.time()

    while time.time() - t0 < timeout:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("ascii", errors="ignore").strip()

        if line == "SCREENSHOT_START":
            collecting = True
            b64_chunks = []
            print("  Capture started...")
            continue
        elif line == "SCREENSHOT_END":
            print(f"  Capture complete ({len(b64_chunks)} chunks)")
            break
        elif line.startswith("SCREENSHOT_ERROR"):
            raise RuntimeError(line)
        elif collecting and line:
            b64_chunks.append(line)
    else:
        raise TimeoutError("Timeout waiting for screenshot data")

    b64_data = "".join(b64_chunks)
    return base64.b64decode(b64_data)


def capture_reset(com_port):
    """Reset device and wait for auto-screenshot after WiFi connects."""
    ser = serial.Serial(com_port, baudrate=115200, timeout=1)
    time.sleep(0.3)
    # Reset via RTS
    ser.dtr = False
    ser.rts = True
    time.sleep(0.1)
    ser.rts = False
    ser.reset_input_buffer()
    print("Device reset, waiting for auto-screenshot...")
    pbm = receive_screenshot(ser, timeout=20)
    ser.close()
    return pbm


def capture_listen(com_port, timeout=45):
    """Listen for button-triggered screenshot."""
    ser = serial.Serial(com_port, baudrate=115200, timeout=1)
    time.sleep(0.3)
    ser.reset_input_buffer()
    print("Listening... press BOOT button on device.")
    pbm = receive_screenshot(ser, timeout=timeout)
    ser.close()
    return pbm


def capture_shoot(com_port, timeout=15):
    """Send SHOOT command (may not work on USB-Serial/JTAG)."""
    ser = serial.Serial(com_port, baudrate=115200, timeout=1, write_timeout=5)
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.write(b"SHOOT\n")
    ser.flush()
    pbm = receive_screenshot(ser, timeout=timeout)
    ser.close()
    return pbm


def main():
    args = sys.argv[1:]
    mode = "reset"
    if "--listen" in args:
        mode = "listen"
        args.remove("--listen")
    elif "--shoot" in args:
        mode = "shoot"
        args.remove("--shoot")
    elif "--reset" in args:
        args.remove("--reset")

    com_port = args[0] if len(args) > 0 else "COM4"
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")

    print(f"Connecting to {com_port} (mode: {mode})...")

    if mode == "reset":
        pbm_data = capture_reset(com_port)
    elif mode == "shoot":
        pbm_data = capture_shoot(com_port)
    else:
        pbm_data = capture_listen(com_port)

    print(f"Received {len(pbm_data)} bytes of PBM data")

    if HAS_PIL:
        out_file = args[1] if len(args) > 1 else f"screenshot_{ts}.png"
        img = Image.open(io.BytesIO(pbm_data))
        img = img.convert("L")
        img.save(out_file)
        print(f"Saved: {out_file} ({img.size[0]}x{img.size[1]})")
    else:
        out_file = args[1] if len(args) > 1 else f"screenshot_{ts}.pbm"
        with open(out_file, "wb") as f:
            f.write(pbm_data)
        print(f"Saved: {out_file} (PBM format)")
        print("Tip: install Pillow for PNG: pip install Pillow")


if __name__ == "__main__":
    main()
