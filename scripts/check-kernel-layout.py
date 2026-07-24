#!/usr/bin/env python3
import subprocess
import sys


KERNEL_VIRTUAL_BASE = 0xC0000000


def read_symbols(path):
    output = subprocess.check_output(["readelf", "-s", path], text=True)
    symbols = {}

    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 8:
            try:
                symbols[fields[-1]] = int(fields[1], 16)
            except ValueError:
                continue

    return symbols


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-kernel-layout.py KERNEL")

    symbols = read_symbols(sys.argv[1])
    start = symbols.get("_start")
    kmain = symbols.get("kmain")

    if start is None or start >= KERNEL_VIRTUAL_BASE:
        raise SystemExit("bootstrap _start is not linked in low memory")
    if kmain is None or kmain < KERNEL_VIRTUAL_BASE:
        raise SystemExit("kmain is not linked in the higher half")


if __name__ == "__main__":
    main()
