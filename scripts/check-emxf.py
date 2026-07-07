#!/usr/bin/env python3
import struct
import subprocess
import sys


USER_CODE_ADDR = 0x00400000


def read_symbol(path, name):
    output = subprocess.check_output(["readelf", "-s", path], text=True)
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 8 and fields[-1] == name:
            return int(fields[1], 16)
    raise SystemExit(f"missing symbol: {name}")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: check-emxf.py FILE.emxf FILE.elf")

    emxf_path = sys.argv[1]
    elf_path = sys.argv[2]

    with open(emxf_path, "rb") as f:
        data = f.read()

    if len(data) < 13:
        raise SystemExit("EMXF file is too small")
    if data[0:4] != b"EMXF":
        raise SystemExit("missing EMXF magic")

    code_size, entry_offset = struct.unpack_from("<II", data, 4)
    if code_size != len(data) - 12:
        raise SystemExit("EMXF code size does not match payload")
    if entry_offset != 0:
        raise SystemExit("EMXF entry offset is not zero")

    start = read_symbol(elf_path, "_start")
    if start != USER_CODE_ADDR:
        raise SystemExit("_start is not linked at 0x00400000")


if __name__ == "__main__":
    main()
