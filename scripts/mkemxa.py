#!/usr/bin/env python3
import struct
import sys

HEADER_SIZE = 8
ENTRY_SIZE = 40
PATH_SIZE = 32


def usage():
    print("usage: mkemxa.py OUTPUT PATH INPUT [PATH INPUT ...]", file=sys.stderr)
    return 1


def main():
    if len(sys.argv) < 4 or len(sys.argv) % 2 != 0:
        return usage()

    output = sys.argv[1]
    files = []

    for i in range(2, len(sys.argv), 2):
        path = sys.argv[i].encode("ascii")
        input_path = sys.argv[i + 1]

        if len(path) >= PATH_SIZE:
            print(f"path too long: {sys.argv[i]}", file=sys.stderr)
            return 1

        with open(input_path, "rb") as f:
            files.append((path, f.read()))

    data_offset = HEADER_SIZE + ENTRY_SIZE * len(files)
    archive = bytearray(b"EMXA" + struct.pack("<I", len(files)))
    data = bytearray()

    for path, contents in files:
        path_field = path + bytes(PATH_SIZE - len(path))
        archive += path_field
        archive += struct.pack("<II", data_offset + len(data), len(contents))
        data += contents

    archive += data

    with open(output, "wb") as f:
        f.write(archive)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
