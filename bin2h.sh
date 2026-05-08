#!/bin/sh
# Convert a binary BIOS image to a header fragment suitable for inclusion
# in bios.cpp (the header will contain the comma-separated byte list).
# Usage: ./bin2h.sh input.bin output.h

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 input.bin output.h"
  exit 1
fi

xxd -i -c 256 "$1" | sed '1d;$d' | sed '$d' > "$2"

echo "Wrote $2 (include this file inside the biosrom[] initializer)"
