#!/usr/bin/env bash
# Belegt die Flush-Politik aus ssd1677_driver.cpp/DisplayTask am Host.
# Handgriff zu den entsprechenden Zeilen in docs/PRUEFUNG.md.
set -euo pipefail
cd "$(dirname "$0")"
g++ -std=c++17 -o /tmp/flush-politik-test flush-politik-test.cpp
exec /tmp/flush-politik-test
