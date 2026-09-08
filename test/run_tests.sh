#!/usr/bin/env bash
set -euo pipefail
perseus_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
perseus_build="$(mktemp -d)"
trap 'rm -rf "$perseus_build"' EXIT
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -Wconversion -Wshadow -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    -DARDUINO_ARCH_ESP32 \
    -I"$perseus_root/tests/fakes" -I"$perseus_root/src" \
    "$perseus_root/src/Perseus.cpp" "$perseus_root/tests/test_perseus.cpp" \
    -x c++ "$perseus_root/examples/BasicRead/BasicRead.ino" \
    -o "$perseus_build/test_perseus"
"$perseus_build/test_perseus"
