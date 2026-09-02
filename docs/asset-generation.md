# Asset-Generierung

Die Pipeline zur Generierung der E-Paper-UI-Assets liegt in diesem Repo,
damit die App-UI generierte monochrome Bild- und Bitmap-Font-Assets nutzen
kann.

## Quell-Verzeichnisse

- `assets/icons/`: Quell-PNG-Icons für Icon-Assets mit fester Größe.
- `assets/logos/`: Quell-PNG-Logos, die das Seitenverhältnis der Quelle beibehalten.
- `fonts/`: Inter-TTF-Quelldateien für gepackte Bitmap-Fonts.

Die aktuellen Skripte sind auf macOS ausgerichtet, weil sie `sips` für die
PNG-Konvertierung und CoreGraphics/CoreText für die Font-Rasterisierung
verwenden.

## Skripte

- `scripts/generate_epaper_icons.py`
- `scripts/generate_epaper_footer_icons.py`
- `scripts/generate_epaper_logos.py`
- `scripts/generate_epaper_fonts.py`
- `scripts/generate_epaper_assets_common.py`
- `scripts/generate_epaper_project_assets.py`

Die generierten Bild-Assets hängen von
`components/project_assets/asset_types.h` ab, die den gemeinsamen gepackten
monochromen Bildtyp definiert.

## Projekt-Asset-Manifest

Der skalierbare Pfad ist `assets/epaper_assets.json`. Neue Quell-Assets
dort eintragen, dann die Projekt-Asset-Komponente neu generieren mit:

```bash
python3 scripts/generate_epaper_project_assets.py
```

Dieser Befehl generiert neu:

- `components/project_assets/asset_manifest.h`
- `components/project_assets/project_assets.h`
- `components/project_assets/project_assets.cpp`
- `components/project_assets/generated_epaper_icons.h/.cpp`
- `components/project_assets/generated_epaper_footer_icons.h/.cpp`
- `components/project_assets/generated_epaper_logos.h/.cpp`

`components/project_assets/CMakeLists.txt` kompiliert bereits alle
generierten Bildquellen, einschließlich der leeren generierten
Icon-/Footer-Dateien. Das Hinzufügen künftiger Icons sollte keine
Änderung an CMake erfordern.

## Eingebettete Bitmap-Fonts

Die Bitmap-Font-Generierung wird jetzt über `components/epaper_ui/`
kompiliert:

- `components/epaper_ui/generated_epaper_fonts.cpp`
- `components/epaper_ui/include/epaper_ui/generated_epaper_fonts.h`

Der aktuelle Renderer nutzt dieselben Inter-Font-Größen und -Schnitte,
die von den portierten `followup`-Design-Token-Rollen erwartet werden.

Die Font-Quelle mit `scripts/generate_epaper_fonts.py` neu generieren,
danach den kleinen öffentlichen Header synchron halten, falls neue
Symbole hinzukommen.

## Eingebettete Logo-Assets

Logo-Assets werden aktuell über `components/project_assets/` eingebettet:

- `EmbeddedLogoId::kAlxvLabsLogo`
- `EmbeddedLogoId::kFollowupLogo`

Sie sind in `assets/epaper_assets.json` gelistet. Neu generieren mit:

```bash
python3 scripts/generate_epaper_project_assets.py
```

UI-Code sollte `project_assets.h` einbinden und
`project_assets::GetLogo(...)` verwenden, statt generierte Dateien direkt
einzubinden.

## Eingebettete Icon-Assets

Alle PNG-Dateien in `assets/icons/` werden aktuell als monochrome
E-Paper-Icon-Assets mit fester Größe `36x36` über `EmbeddedIconId` und
`project_assets::GetIcon(...)` eingebettet.

So fügst du ein neues Icon hinzu:

1. Das Quell-PNG nach `assets/icons/` hinzufügen.
2. Einen Eintrag im `icons`-Array in `assets/epaper_assets.json` ergänzen.
3. `python3 scripts/generate_epaper_project_assets.py` ausführen.

Beispiel:

```bash
python3 scripts/generate_epaper_fonts.py \
  --output components/epaper_ui/generated_epaper_fonts.cpp \
  fonts/Inter_18pt-SemiBold.ttf:kInter22SemiBold:22 \
  fonts/Inter_18pt-Bold.ttf:kInter22Bold:22 \
  fonts/Inter_18pt-Black.ttf:kInter22Black:22 \
  fonts/Inter_18pt-SemiBold.ttf:kInter26SemiBold:26 \
  fonts/Inter_18pt-Bold.ttf:kInter26Bold:26 \
  fonts/Inter_18pt-Black.ttf:kInter26Black:26 \
  fonts/Inter_24pt-SemiBold.ttf:kInter32SemiBold:32 \
  fonts/Inter_24pt-Bold.ttf:kInter32Bold:32 \
  fonts/Inter_24pt-Black.ttf:kInter32Black:32 \
  fonts/Inter_24pt-SemiBold.ttf:kInter38SemiBold:38 \
  fonts/Inter_24pt-Bold.ttf:kInter38Bold:38 \
  fonts/Inter_24pt-Black.ttf:kInter38Black:38 \
  fonts/Inter_24pt-SemiBold.ttf:kInter46SemiBold:46 \
  fonts/Inter_24pt-Bold.ttf:kInter46Bold:46 \
  fonts/Inter_24pt-Black.ttf:kInter46Black:46 \
  fonts/Inter_28pt-SemiBold.ttf:kInter55SemiBold:55 \
  fonts/Inter_28pt-Bold.ttf:kInter55Bold:55 \
  fonts/Inter_28pt-Black.ttf:kInter55Black:55 \
  fonts/Inter_28pt-SemiBold.ttf:kInter165SemiBold:165 \
  fonts/Inter_28pt-Bold.ttf:kInter165Bold:165 \
  fonts/Inter_28pt-Black.ttf:kInter165Black:165
```

## Richtlinie

Die PNG- und TTF-Dateien als kanonische Quell-Assets behandeln. Generierte
C++-Dateien sollten aus diesen Eingaben und den Skripten reproduzierbar
sein und nicht von Hand bearbeitet werden.
