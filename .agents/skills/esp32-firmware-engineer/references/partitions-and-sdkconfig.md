# ESP32-Partitionen und sdkconfig (ESP-IDF)

Diese Referenz nutzen beim Ändern von Flash-Layout, OTA-Unterstützung oder Projekt-Konfiguration.

## Kernregeln

- `sdkconfig` und Partitions-CSV als vollwertige Projekt-Artefakte behandeln.
- Bearbeiten der Konfigurationsdateien (`sdkconfig`, `sdkconfig.defaults`, Kconfig-Fragmente, wo genutzt) gegenüber interaktiven `menuconfig`-Anweisungen bevorzugen, für Reproduzierbarkeit.
- Keine Partitionsänderungen vorschlagen, bevor Flash-Größe und OTA-Anforderung bestätigt sind.
- Verfügbare Flash-Kapazität bewusst nutzen; unerklärte leere Regionen vermeiden.

## Was vor Partitionsänderungen zu bestätigen ist

- Exakte Flash-Größe (zum Beispiel 4 MB, 8 MB, 16 MB)
- OTA-Anforderung (einzelne App vs. Dual-Slot-OTA, Rollback-Bedarf)
- NVS-Größenbedarf (WLAN-Zugangsdaten, App-Konfiguration, Kalibrierungsdaten)
- Dateisystem-/Daten-Partitionsbedarf (SPIFFS/LittleFS/FATFS, falls genutzt)
- Core-Dump-Partitionsanforderung (falls aktiviert)
- Factory-App-Partitionsanforderung (manche Produkte brauchen sie; viele nicht)

## Partitionsstrategie-Leitlinien

### Keine OTA erforderlich

- Eine größere App-Partition plus angemessen dimensionierte NVS-/Daten-Partitionen bevorzugen.
- OTA-Slots nicht reservieren, außer sie werden tatsächlich gebraucht.

### OTA erforderlich

- OTA-kompatibles Layout nutzen (typischerweise `otadata` + zwei OTA-App-Slots).
- OTA-Slots basierend auf aktueller Binary-Größe plus Wachstums-Spielraum dimensionieren.
- Sicherstellen, dass Partitionswahl mit `sdkconfig`-OTA- und Bootloader-Einstellungen übereinstimmt.
- Wird Rollback genutzt, sicherstellen, dass Konfiguration und Partitionierung es unterstützen.

## Flash-Nutzungsrichtlinie

- Jede Partition sollte einen Grund haben.
- Freier Speicher sollte entweder:
  - als Wachstums-Spielraum mit explizitem Hinweis zugewiesen sein, oder
  - für nützliche Daten-/App-Kapazität vergeben sein.
- Keine großen Lücken durch kopierte Beispiel-Layouts belassen, die nicht zum Ziel-Flash passen.

## sdkconfig-Bearbeitungs-Workflow (reproduzierbar)

- Aktuelle `sdkconfig` und relevante Komponenten-/Projekt-Defaults lesen.
- Nur die erforderlichen Schlüssel ändern.
- Zusammenhängende Einstellungen synchron halten (Beispiel: Target, Flash-Größe, Log-Level, PSRAM, Partitionstabellen-Optionen).
- Erklären, warum jede Konfigurationsänderung vorgenommen wurde.
- Prüfen des resultierenden `sdkconfig`-Diffs gegenüber vagen Menü-Navigationsschritten bevorzugen.

## menuconfig-Richtlinie

- `menuconfig` ist ein Erkundungs-/Debug-Werkzeug, nicht das primäre Liefer-Artefakt.
- Wird `menuconfig` genutzt, um eine Option zu finden: die finale Änderung in `sdkconfig`/Defaults widerspiegeln und die exakten Konfigurationsschlüssel zeigen.
- Den Nutzer nicht nur mit "menuconfig öffnen und X klicken"-Anweisung zurücklassen, außer explizit angefragt.

## Partitions-/Konfigurations-Review-Checkliste

- Exakte Flash-Größe bestätigt.
- OTA-Anforderung bestätigt.
- Partitionstabelle passt zu Feature-Set und Flash-Kapazität.
- App-Slot-Größen enthalten realistischen Spielraum.
- NVS-/Daten-/Core-Dump-Partitionen bewusst dimensioniert.
- `sdkconfig`-Partitionstabellen-Auswahl zeigt auf die richtige CSV.
- Log-Level-, PSRAM- und Flash-/Boot-Einstellungen sind konsistent mit Performance-/Debug-Zielen.
- Kein kopiertes Beispiel-Layout bleibt ohne Begründung bestehen.
