# ESP32-Firmware-Engineering-Werte

Diese Datei zuerst nutzen. Sie definiert nicht verhandelbares Verhalten für den ESP32-Firmware-Skill.

## 1. Hardware-Wahrheit vor Code

- `Wert`: Die Firmware muss zur tatsächlichen Hardware passen, nicht zu einem angenommenen Board.
- `Warum`: Die meisten Embedded-Fehlschläge kommen von falschen Pin-Belegungen, elektrischen Annahmen oder fehlendem Peripherie-Kontext.
- `Tun`:
  - Exakte ESP32-Variante bestätigen.
  - Peripherie-Inventar, Verkabelung, Busse, Power-Rails, Transceiver und Display-/Controller-Details bestätigen.
  - Vor Implementierung oder Debugging nach fehlenden Informationen fragen.
- `Vermeiden`:
  - Pins, Interfaces, Pull-ups, Spannungspegel oder Controller-Modelle raten.
  - Generische Beispiele direkt in ESP-IDF-Projekte portieren.
- `Blockierende Regel`: Ist der Hardware-Kontext für hardware-nahe Aufgaben unvollständig, stoppen und nachfragen.

## 2. Varianten-Gewissheit ist verpflichtend

- `Wert`: Nie mit einem unbekannten ESP32-Ziel fortfahren.
- `Warum`: ESP32-Varianten unterscheiden sich materiell (Kerne, Peripherie, Speicher, Funk-/Peripherie-Fähigkeiten, Schlaf-/Aufwach-Funktionen).
- `Tun`: Exaktes Ziel (`esp32`, `esp32s3`, `esp32c3` usw.) und ESP-IDF-Version verlangen.
- `Vermeiden`: Code schreiben, der Dual-Core-Verhalten, Peripherie-Verfügbarkeit oder Aufwach-Funktionen über Varianten hinweg annimmt.
- `Review-Hinweise`:
  - Nimmt der Code eine Peripherie an, die auf dem Ziel nicht vorhanden ist?
  - Nimmt er Dual-Core-/Pinning an, wo der Chip Single-Core ist?

## 3. Konfiguration und Partitionen sind Quellcode

- `Wert`: `sdkconfig` und Partitionstabellen sind Teil des Lieferergebnisses, kein Nachgedanke.
- `Warum`: Viele Laufzeit-Fehlschläge und Performance-Probleme sind konfigurations-, nicht code-getrieben.
- `Tun`:
  - `sdkconfig`/`sdkconfig.defaults` bewusst und reproduzierbar bearbeiten.
  - Partitionen an den tatsächlichen Flash und Funktionsumfang anpassen.
  - OTA-kompatible Layouts nutzen, wenn OTA erforderlich ist.
  - Unerklärte ungenutzte Flash-Kapazität vermeiden.
- `Vermeiden`:
  - "Menuconfig nutzen und herumklicken" als primären Arbeitsablauf.
  - Partitions-Ratereien ohne Flash-Größen-/OTA-Anforderungen.

## 4. Nur build-erwiesene Änderungen

- `Wert`: Eine Änderung ist erst fertig, wenn das Projekt im Projekt-Workflow sauber baut.
- `Warum`: Embedded-Fehlschläge zeigen sich oft in generierter Konfiguration, der Link-Phase oder Warnungen, die echte Bugs anzeigen.
- `Tun`:
  - Vor dem Build-Schritt bestätigen, dass ESP-IDF-Tooling installiert und nutzbar ist (nicht nur "wahrscheinlich installiert").
  - Nach Änderungen den projekteigenen `build.sh` ausführen (oder einen gleichwertigen Build-Wrapper).
  - Fehlschläge beheben und erneut ausführen, bis er besteht.
  - Wichtige Warnungen als zu erledigende Arbeit behandeln, nicht als Rauschen.
- `Vermeiden`: Fertigstellung allein aufgrund von Überlegungen oder teilweiser Kompilierung erklären.

## 5. Kompatibilitätsnachweis vor Fortschritt

- `Wert`: Versions-Kompatibilität ist eine Nachweispflicht, kein Raten.
- `Warum`: ESP-IDF-+-ESP-ADF-+-ESP-SR-Stacks können scheitern, wenn Versionen nicht exakt unterstützten Kombinationen entsprechen.
- `Tun`:
  - Exakte Versionen/Tags/Commits für jedes genutzte Plugin/Framework identifizieren.
  - Konkrete Belege aus offiziellen Matrizen/Manifesten/Release-Notes sammeln.
  - Explizite Cross-Stack-Belege verlangen, wenn mehrere Frameworks zusammenwirken (z. B. ADF + SR).
- `Vermeiden`:
  - Annehmen, dass "neueste mit neuester" kompatibel ist.
  - Ohne Versionsnachweis auf Basis von Anekdoten fortfahren.
- `Blockierende Regel`: Ist ein Plugin-Kompatibilitäts-Zusammenhang unbewiesen, vor Build/Debug/Flash stoppen.

## 6. Beobachtbarkeit mit hohem Signal

- `Wert`: Rauschen unterdrücken, Anwendungs-Signal erhöhen.
- `Warum`: Embedded-Debugging hängt von Logs ab, aber laute Standardeinstellungen verdecken Kausalität.
- `Tun`:
  - Irrelevante Bibliotheks-/Standard-Komponenten-Logs bei der Diagnose reduzieren.
  - Anwendungs-Logs ausführlich, getaggt und zustandsbewusst halten.
  - Fehlercodes, Wiederholversuche, Timings und Übergänge loggen.
- `Vermeiden`:
  - Globale Log-Unterdrückung, die Belege entfernt.
  - Generische "failed"-Logs ohne Kontext.

## 7. Serviceability standardmäßig (wenn Konsolen-Transport frei ist)

- `Wert`: Ist ein USB-/Serial-Konsolenpfad verfügbar und ungenutzt, standardmäßig ein einfaches Terminal ausliefern.
- `Warum`: Laufzeit-Inspektion und Einstellungs-Kontrolle reduzieren Debug-Iterationszeit und Feld-Diagnose-Aufwand drastisch.
- `Tun`:
  - ESP-IDF-Konsole/REPL mit Hilfe, Verlauf und Autovervollständigung nutzen.
  - Sichere Kommandos für Einstellungen, Status, Heap, RTOS-Diagnostik und Log-Level bereitstellen.
  - Kommando-Handler begrenzt und nutzerfreundlich halten.
- `Vermeiden`:
  - Ad-hoc-Parser mit schlechten Fehlermeldungen.
  - Nur-Debug-Kommandos, die das Timing destabilisieren oder Geheimnisse offenlegen.
- `Blockierende Regel`: Transport-Eigentümerschaft und Sicherheitsrichtlinie bestätigen, bevor das Terminal aktiviert wird.

## 8. Korrektes Datenformat zuerst (besonders bei Displays)

- `Wert`: Datenformat-Korrektheit kommt vor Grafiklogik oder Performance-Tuning.
- `Warum`: Display-Bugs sind oft Pixelformat-, Byte-Reihenfolge-, Stride- oder Controller-Init-Fehlanpassungen.
- `Tun`:
  - Controller, Bus-Modus, Auflösung, Pixelformat, Endian-/Farbreihenfolge und Flush-Bereichsformat bestätigen.
  - Nur in das exakte Format konvertieren, das der Display-Pfad erwartet.
- `Vermeiden`:
  - RGB-/BGR-Reihenfolge raten oder RGB565-Packing annehmen.
  - Farbprobleme durch zufälliges Bit-Tauschen "fixen".

## 9. Performance-Entscheidungen müssen Hardware-Grenzen respektieren

- `Wert`: Die performanteste zuverlässige Option nutzen, die von der tatsächlichen Hardware unterstützt wird.
- `Warum`: Durchsatz hängt von Bus-Geschwindigkeit, DMA-Fähigkeit, Speicherplatzierung, Flash-/PSRAM-Modi und Signalintegrität ab.
- `Tun`:
  - RAM-/Flash-/PSRAM-Fähigkeiten und Bus-Timing-Grenzen prüfen.
  - DMA-fähigen Speicher nutzen, wo erforderlich.
  - Erst messen, dann tunen.
- `Vermeiden`:
  - Benchmark-Annahmen, die von Board-Verkabelung und Takt-Konfiguration losgelöst sind.
  - Geschwindigkeit mit instabilen Einstellungen erjagen.

## 10. Explizite Unbekannte, explizite Risiken

- `Wert`: Festhalten, was in der Hardware bekannt, unbekannt und unverifiziert ist.
- `Warum`: Embedded-Software kann im Code-Review korrekt wirken und trotzdem auf echten Boards scheitern.
- `Tun`: Fehlende Hardware-Validierung und verbleibende Annahmen in der finalen Antwort benennen.
- `Tun`: Nach Beispielcode fragen (Projekt-Snippet, bekannt-gute Implementierung oder minimaler Repro-Fall), wenn Unsicherheit sonst zum Raten zwingen würde.
- `Vermeiden`: Hardware-Verifikation suggerieren, wenn nur Build-/Log-Review durchgeführt wurde.
- `Vermeiden`: Lücken mit geratener API-Nutzung oder angenommenem Verhalten füllen, wenn stattdessen ein Beispiel angefragt werden kann.
