#!/usr/bin/env bash
# v2 des Host-Monkey-Tests: wie run.sh, aber mit einem GROSSEN, realistischen
# Startset (~450 gemischte Eintraege: Aufgaben/Notizen/Ideen ueber viele Tage).
# Damit laeuft der Test gegen viele Gruppen und lange Listen -- naeher am
# echten Geraet nach Wochen Nutzung.
#
# Aufruf:  bash run_v2.sh [STARTSET]     (Default 450)
set -euo pipefail

HIER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HIER/../.." && pwd)"
OUT="$HIER/todos_monkey"
STARTSET="${1:-450}"

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
  -I"$HIER/stubs/idf"
)

QUELLEN=(
  "$HIER/todos_monkey.cpp"
  "$REPO/main/todos_page_coordinator.cpp"
  "$REPO/main/todos_page_interactions.cpp"
  "$REPO/main/timeline_format.cpp"
  "$REPO/components/page_navigation/navigation_model.cpp"
  "$REPO/components/page_navigation/roving_focus.cpp"
  "$HIER/stubs/project_assets_stub.cpp"
  "$HIER/stubs/timezone_service_stub.cpp"
)

echo "== Kompiliere Host-Monkey-Test (v2) =="
g++ -std=c++20 -O2 -Wall -Wextra -g \
    "${INCLUDES[@]}" "${QUELLEN[@]}" -o "$OUT"

echo "== Lauf (Startset ~$STARTSET Eintraege) =="
STATUS=0
"$OUT" 20260921 2000000 "$STARTSET" || STATUS=$?
for s in 1 7 42 1234 99991 20250101; do
  "$OUT" "$s" 2000000 "$STARTSET" || STATUS=$?
done
exit "$STATUS"
