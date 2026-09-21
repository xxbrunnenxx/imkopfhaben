#!/usr/bin/env bash
# Baut den Host-Monkey-Test der Todos-Navigation und fuehrt ihn aus.
# Kompiliert die ECHTEN Logikquellen (Coordinator, Interactions,
# NavigationModel, RovingFocus, timeline_format) gegen zwei Host-Stubs
# (project_assets -> nullptr, timezone_service -> festes Datum).
set -euo pipefail

HIER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HIER/../.." && pwd)"
OUT="$HIER/todos_monkey"

INCLUDES=(
  -I"$REPO/main"
  -I"$REPO/components/page_navigation/include"
  -I"$REPO/components/epaper_ui/include"
  -I"$REPO/components/design_tokens/include"
  -I"$REPO/components/project_assets"
  -I"$REPO/components/recording_archive_service/include"
  -I"$REPO/components/recording_service/include"
  -I"$REPO/components/display_service/include"
  -I"$REPO/components/timezone_service/include"
  -I"$HIER/stubs/idf"     # minimale ESP-IDF-Ersatzheader (esp_err.h, esp_http_server.h)
)

# Echte Logik -- keine Hardware, kein Display.
QUELLEN=(
  "$HIER/todos_monkey.cpp"
  "$REPO/main/todos_page_coordinator.cpp"
  "$REPO/main/todos_page_interactions.cpp"
  "$REPO/main/timeline_format.cpp"
  "$REPO/components/page_navigation/navigation_model.cpp"
  "$REPO/components/page_navigation/roving_focus.cpp"
  # Host-Stubs:
  "$HIER/stubs/project_assets_stub.cpp"
  "$HIER/stubs/timezone_service_stub.cpp"
)

echo "== Kompiliere Host-Monkey-Test =="
g++ -std=c++20 -O2 -Wall -Wextra -g \
    "${INCLUDES[@]}" "${QUELLEN[@]}" -o "$OUT"

echo "== Lauf =="
# Erst der feste Seed (reproduzierbar), dann eine Handvoll zufaelliger Seeds
# fuer Breite. Jeder Lauf 2 Mio Schritte.
STATUS=0
"$OUT" 20260921 2000000 || STATUS=$?
for s in 1 7 42 1234 99991 20250101; do
  "$OUT" "$s" 2000000 || STATUS=$?
done
exit "$STATUS"
