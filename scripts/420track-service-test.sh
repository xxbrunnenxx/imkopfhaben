#!/usr/bin/env bash
# Belegt am Host, dass der ECHTE joint_tracker_service (components/
# joint_tracker_service/joint_tracker_service.cpp) den Mitternachts-Reset und
# die Zaehl-Logik richtig macht. Handgriff zu docs/PRUEFUNG.md, 420-Track.
#
# Die Uhr wird ueber den Linker (--wrap=time) gesteuert, NVS durch einen
# In-Memory-Stub ersetzt (scripts/420track-stub/). Der Service selbst ist
# unveraendert der Geraetecode. Getestet:
#   - Tageswechsel setzt den Zaehler auf 0, gestriger Stand wandert ins Protokoll
#   - Reset loest auch ohne Tastendruck aus (nur ueber den Getter)
#   - Increment/Decrement, kein Negativ, ueber Ziel erlaubt (nicht gedeckelt)
set -euo pipefail
cd "$(dirname "$0")/.."

SVC=components/joint_tracker_service
BIN=/tmp/420track-service-test

g++ -std=c++17 -O1 \
  -I "$SVC/include" -I scripts/420track-stub \
  -Wl,--wrap=time \
  scripts/420track-service-test.cpp "$SVC/joint_tracker_service.cpp" \
  -o "$BIN"

# Abgleich: laufen Richtwert oder History-Laenge zwischen Code und Test
# auseinander, belegt der Test etwas anderes als das Geraet -- dann scheitern.
GOAL_CODE=$(grep -oE 'kDailyGoal = [0-9]+' "$SVC/include/joint_tracker_service.h" | grep -oE '[0-9]+')
GOAL_TEST=$(grep -oE 'goal 4|Ziel 4|Richtwert ist 4' scripts/420track-service-test.cpp | head -1 | grep -oE '[0-9]+')
if [ "$GOAL_CODE" != "$GOAL_TEST" ]; then
    echo "ABGLEICH FEHLGESCHLAGEN: Richtwert Code=$GOAL_CODE Test=$GOAL_TEST" >&2
    exit 1
fi
echo "Abgleich Richtwert: Code=$GOAL_CODE Test=$GOAL_TEST"

"$BIN"
