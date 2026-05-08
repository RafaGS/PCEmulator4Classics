#!/bin/sh

nasm bios.asm -o bios.bin
xxd -i -c 256 bios.bin | sed '1d;$d' | sed '$d' > biosrom.h
rm -f bios.bin
