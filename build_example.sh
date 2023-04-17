#!/bin/sh

set -xe

make

CC=gcc

$CC -E example/lines.c -o - | ./main /dev/stdin | $CC -Wno-incompatible-pointer-types -c -x c - -o build/example_lines.o
$CC -E example/main.c -o - | ./main /dev/stdin | $CC -Wno-incompatible-pointer-types -c -x c - -o build/example_main.o
$CC -E example/circle.c -o - | ./main /dev/stdin | $CC -Wno-incompatible-pointer-types -c -x c - -o build/example_circle.o
$CC build/example_lines.o build/example_circle.o build/example_main.o -o build/example
