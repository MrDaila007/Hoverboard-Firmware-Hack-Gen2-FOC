#!/usr/bin/env python3
"""Fail if firmware.elf exceeds GD32F130C8 flash/RAM limits."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

FLASH_MAX = 65536
RAM_MAX = 8192


def find_size_tool() -> str:
    home = Path.home()
    candidates = sorted(home.glob(".platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-size"))
    if candidates:
        return str(candidates[-1])
    return "arm-none-eabi-size"


def parse_size_output(output: str) -> tuple[int, int]:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    data_line = lines[-1]
    parts = data_line.split()
    if len(parts) < 3:
        raise ValueError(f"unexpected size output: {output!r}")
    text = int(parts[0], 10)
    data = int(parts[1], 10)
    bss = int(parts[2], 10)
    return text + data, data + bss


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <firmware.elf>", file=sys.stderr)
        return 2

    elf = Path(sys.argv[1])
    if not elf.is_file():
        print(f"error: firmware not found: {elf}", file=sys.stderr)
        return 1

    size_tool = find_size_tool()
    result = subprocess.run(
        [size_tool, "-B", "-d", str(elf)],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(result.stderr or result.stdout, file=sys.stderr)
        return 1

    flash_used, ram_used = parse_size_output(result.stdout)
    ok = True

    print(f"Flash: {flash_used} / {FLASH_MAX} bytes")
    print(f"RAM:   {ram_used} / {RAM_MAX} bytes")

    if flash_used > FLASH_MAX:
        print(f"error: flash usage exceeds {FLASH_MAX} bytes", file=sys.stderr)
        ok = False
    if ram_used > RAM_MAX:
        print(f"error: RAM usage exceeds {RAM_MAX} bytes", file=sys.stderr)
        ok = False

    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
