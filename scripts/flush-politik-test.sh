#!/usr/bin/env bash
# Belegt die Flush-Politik aus ssd1677_driver.cpp/display_service.cpp am Host.
# Handgriff zu den entsprechenden Zeilen in docs/PRUEFUNG.md.
set -euo pipefail
cd "$(dirname "$0")/.."

TREIBER=components/epaper_panel/ssd1677_driver.cpp
DIENST=components/display_service/display_service.cpp
TEST=scripts/flush-politik-test.cpp

# Der Test bildet den Treiber nach. Laufen die Konstanten auseinander, belegt er
# etwas anderes als das, was auf dem Geraet laeuft -- dann lieber lautstark
# scheitern als still das Falsche bestaetigen.
abgleich() {
    local was="$1" datei="$2" muster="$3"
    local echt test
    echt=$(grep -oE "$muster" "$datei" | head -1 | grep -oE '[0-9]+')
    test=$(grep -oE "$muster" "$TEST" | head -1 | grep -oE '[0-9]+')
    if [ -z "$echt" ] || [ -z "$test" ]; then
        echo "ABGLEICH FEHLGESCHLAGEN: $was nicht gefunden (echt='$echt' test='$test')" >&2
        exit 1
    fi
    if [ "$echt" != "$test" ]; then
        echo "ABGLEICH FEHLGESCHLAGEN: $was ist im Code $echt, im Test $test" >&2
        exit 1
    fi
    printf '%-58s %s\n' "Abgleich $was" "Code=$echt Test=$test"
}

echo "== Abgleich Test gegen echten Code =="
abgleich "weiche Schwelle"  "$TREIBER" 'kMaxPartialRefreshesBeforeFlush = [0-9]+'
abgleich "harte Grenze"     "$TREIBER" 'kMaxPartialRefreshesHardCap = [0-9]+'
echo

g++ -std=c++17 -Wall -Wextra -o /tmp/flush-politik-test "$TEST"
exec /tmp/flush-politik-test
