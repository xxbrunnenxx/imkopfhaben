#!/usr/bin/env bash
# Belegt am Host, dass die ECHTE 420-Track-Karte (components/epaper_ui/
# joint_tracker_card.cpp) die Punkte richtig zeichnet: unter Ziel Umriss, bis
# Ziel gefuellt, ueber Ziel gefuellt mit hellem Kern (markiert, nicht
# limitiert). Handgriff zu docs/PRUEFUNG.md, Abschnitt 420-Track.
#
# Kompiliert die echten UI-Quellen (kein Nachbau) und rendert in einen
# 1bpp-Framebuffer wie auf dem Geraet (raw 800x480, portrait 480x800). Danach
# werden die Punkte an ihren bekannten Positionen klassifiziert. Erwartung:
#   count=2,goal=4 -> 2 gefuellt, 2 Umriss
#   count=4,goal=4 -> 4 gefuellt
#   count=6,goal=4 -> 4 gefuellt, 2 mit hellem Kern
set -euo pipefail
cd "$(dirname "$0")/.."

UI=components/epaper_ui
DT=components/design_tokens/include
PA=components/project_assets
BIN=/tmp/420track-render-test

g++ -std=c++20 -O1 \
  -I "$UI/include" -I "$UI" -I "$DT" -I "$PA" \
  scripts/420track-render-test.cpp \
  "$UI/joint_tracker_card.cpp" "$UI/render_utils.cpp" "$UI/font_renderer.cpp" \
  "$UI/bitmap_font.cpp" "$UI/generated_epaper_fonts.cpp" \
  -o "$BIN"

OUT=$("$BIN")
echo "$OUT"

pruefe() {
    local muster="$1" was="$2"
    if ! grep -qF "$muster" <<<"$OUT"; then
        echo "FEHLGESCHLAGEN: $was (Zeile '$muster' fehlt)" >&2
        exit 1
    fi
    echo "  belegt: $was"
}

echo "== Auswertung =="
pruefe "Gemessen: 2 massiv gefuellt, 0 gefuellt-mit-Kern (ueber Limit), 2 Umriss (offen), 0 leer" "count=2: 2 gefuellt + 2 Umriss"
pruefe "Gemessen: 4 massiv gefuellt, 0 gefuellt-mit-Kern (ueber Limit), 0 Umriss (offen), 0 leer" "count=4: 4 gefuellt"
pruefe "Gemessen: 4 massiv gefuellt, 2 gefuellt-mit-Kern (ueber Limit), 0 Umriss (offen), 0 leer" "count=6: 4 gefuellt + 2 ueber Limit markiert"
echo "ALLES BELEGT. Vorschau-PGMs unter /tmp/card*.pgm"
