# Skripte

Dieses Repo hält die E-Paper-Asset-Generatoren, portiert aus dem
Schwester-Repo `followup`.

## E-Paper-Assets

Quell-Grafiken liegen in:

- `assets/icons/`
- `assets/logos/`
- `fonts/`

Die Generatoren erzeugen monochrome C++-Assets für die SSD1677-E-Paper-UI:

- `generate_epaper_icons.py`: Icon-Assets mit fester Größe `36x36`
- `generate_epaper_footer_icons.py`: Footer-Icon-Assets mit fester Größe `44x44`
- `generate_epaper_logos.py`: Logo-Assets mit erhaltenem Seitenverhältnis
- `generate_epaper_fonts.py`: gepackte ASCII-Bitmap-Fonts aus TTF-Dateien
- `generate_epaper_project_assets.py`: manifest-gesteuerter Wrapper für
  alle Projekt-Bild-Assets

Die PNG-Generatoren nutzen macOS `sips`, der Font-Generator nutzt macOS
CoreGraphics/CoreText über `ctypes`.

Für die normale UI-Arbeit `assets/epaper_assets.json` aktualisieren und
ausführen:

```bash
python3 scripts/generate_epaper_project_assets.py
```

Die tiefer liegenden Skripte bleiben für schnelle Einzel-Experimente
verfügbar.

Beispiele:

```bash
python3 scripts/generate_epaper_icons.py \
  --output-header components/project_assets/generated_epaper_icons.h \
  --output-source components/project_assets/generated_epaper_icons.cpp \
  assets/icons/home.png:kHome \
  assets/icons/settings.png:kSettings

python3 scripts/generate_epaper_logos.py \
  --output-header components/project_assets/generated_epaper_logos.h \
  --output-source components/project_assets/generated_epaper_logos.cpp \
  assets/logos/folloup-logo.png:kFollowupLogo

python3 scripts/generate_epaper_fonts.py \
  --output components/epaper_ui/generated_epaper_fonts.cpp \
  fonts/Inter_18pt-SemiBold.ttf:kInter22SemiBold:22
```

Generierte Asset-Dateien nach dem Einspielen nicht von Hand bearbeiten.
Stattdessen die Quell-PNG-/TTF-Dateien aktualisieren und neu generieren.

## Ins Archiv schauen

`archiv-zeigen.py` holt die Aufnahmen und die beiden Zusammenfassungen vom
laufenden Gerät und schreibt sie lesbar untereinander — die Transkripte
mit Uhrzeit, Tag und Dauer, darunter die Zusammenfassungen samt Herkunft
(wie viele Aufnahmen eingeflossen sind, ob gekürzt oder gechunkt wurde).
So lässt sich beurteilen, ob eine Zusammenfassung zu ihren Quellen passt,
ohne sie vom E-Paper abzutippen.

```bash
scripts/archiv-zeigen.py                  # Standardadresse
scripts/archiv-zeigen.py 192.168.178.75   # oder eine andere
```

Es liest nur. Die zugrunde liegenden Routen sind
`GET /api/archive/recordings` und `GET /api/archive/summaries`, beschrieben
in `docs/app-architecture.md`. Anders als die Asset-Generatoren oben
braucht dieses Skript kein macOS — nur die Python-Standardbibliothek und
ein erreichbares Gerät.

## Host-Tests (echter Code, ohne Geraet)

Diese Skripte kompilieren **echte** Geraete-Quellen am Host und belegen ihr
Verhalten, ohne dass ein Board angeschlossen sein muss. Sie sind der Handgriff
zu den entsprechenden Zeilen in `docs/PRUEFUNG.md`.

- `flush-politik-test.sh`: bildet die Flush-Politik aus dem Display-Treiber
  nach und gleicht die Konstanten gegen den echten Code ab.
- `420track-render-test.sh`: rendert die echte 420-Track-Karte
  (`components/epaper_ui/joint_tracker_card.cpp`) in einen 1bpp-Framebuffer
  wie auf dem Geraet und klassifiziert die Punkte (Umriss / gefuellt /
  gefuellt-mit-hellem-Kern) fuer count=2/4/6 gegen Ziel 4. PGM-Vorschau in
  `/tmp/card*.pgm`.
- `420track-service-test.sh`: treibt den echten `joint_tracker_service` ueber
  einen Mitternachts-Tageswechsel (Uhr per Linker-`--wrap=time`, NVS als
  In-Memory-Stub in `420track-stub/`). Belegt Reset, Protokoll-Fortschreibung
  und die Zaehl-Logik (+1/-1, kein Negativ, ueber Ziel erlaubt).

```bash
scripts/420track-render-test.sh
scripts/420track-service-test.sh
```

Beide brauchen nur `g++` (C++17/20), kein ESP-IDF und kein Geraet.
