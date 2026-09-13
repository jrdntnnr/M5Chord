import argparse
from datetime import datetime, timezone
from pathlib import Path
import sys
import time

import serial


def main():
    parser = argparse.ArgumentParser(description="Capture M5Chord diagnostic USB console without resetting the board")
    parser.add_argument("--port", required=True)
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--reconnect", action="store_true")
    args = parser.parse_args()
    if args.seconds <= 0 or args.seconds > 3600:
        parser.error("Duration must be greater than zero and at most 3600 seconds")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as log:
        log.write(f"Captured {datetime.now(timezone.utc).isoformat()} port={args.port}\n")
        with serial.Serial(port=None, baudrate=115200, timeout=0.1, write_timeout=0.5) as connection:
            connection.port = args.port
            connection.dtr = True
            connection.rts = False
            connection.open()
            connection.write(b"l\n")
            if args.reconnect:
                connection.write(b"r\n")
            deadline = time.monotonic() + args.seconds
            while time.monotonic() < deadline:
                data = connection.read(1024)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    log.write(text)
                    log.flush()
                    sys.stdout.write(text)
                    sys.stdout.flush()


if __name__ == "__main__":
    main()
