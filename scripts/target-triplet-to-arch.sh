#!/bin/sh
set -e
triplet="$1"
echo "$triplet" | sed -E 's/-.*$//' | sed 's/i.86/i386/'
