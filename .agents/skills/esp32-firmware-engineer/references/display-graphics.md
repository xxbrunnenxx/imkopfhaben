# ESP32-Display- und Grafik-Validierung (ESP-IDF)

Diese Referenz nutzen für Display-Bring-up, Framebuffer-Formate, Flush-Pfade und Grafik-Korrektheit bei ESP32-Projekten.

## Erstes Prinzip: Den Display-Datenpfad validieren

Vor dem Schreiben oder Ändern von Grafik-Code bestätigen:

- Display-Controller-Modell (zum Beispiel ST7789, ILI9341, GC9A01 usw.)
- Schnittstellentyp (SPI, i80/parallel, RGB, MIPI-DSI auf unterstützten Zielen)
- Auflösung und Ausrichtung
- Vom Controller/Pfad erwartetes Pixelformat (RGB565, BGR565, RGB888 usw.)
- Byte-Reihenfolge / Endianness / Farbreihenfolge
- Fenster-/Flush-Kommandoprotokoll und Regions-Ausrichtungsbeschränkungen
- DMA-/Puffer-Anforderungen (Ausrichtung, internes RAM vs. PSRAM-Unterstützung)

Ist eines davon unbekannt: anhalten und nachfragen, bevor Grafik-Code geändert wird.

## Häufige Fehlerbilder (meist keine "Rendering-Logik"-Bugs)

- Vertauschte Farben (RGB/BGR-Mismatch)
- Blau/Rot vertauscht oder Farbstich-Probleme (Byte-Reihenfolge-/Endian-Mismatch)
- Beschädigte Zeilen/Tearing (Puffer-Stride, DMA-Ausrichtung, Race bei der Flush-Ownership)
- Teil-Updates im falschen Bereich (Fenster-Koordinaten- oder Rotations-Transform-Mismatch)
- Zufällige Beschädigung unter Last (Puffer-Lebensdauer-Problem, PSRAM-/DMA-Mismatch, Cache-/Kohärenz-Annahmen)

## Puffer- und Format-Regeln

- Nur in das exakte Format konvertieren, das der Display-Pfad erwartet.
- Ein einziges, dokumentiertes Source-of-Truth-Format an der Display-Grenze beibehalten.
- Stride-/Zeilen-Pitch-Annahmen explizit validieren.
- Nicht annehmen, dass die Standard-Farbreihenfolge einer Library zur eigenen Panel-/Controller-Konfiguration passt.

## Performance-Überlegungen

- Bus-Takt und DMA-Nutzung an Board-Verkabelung und Panel-Stabilitätsgrenzen anpassen.
- DMA-fähige Puffer für große Transfers bevorzugen, wenn unterstützt/erforderlich.
- Prüfen, ob der Display-Treiber-Pfad PSRAM-gestützte Puffer auf dem gewählten Target und der IDF-Version unterstützt.
- Teil-Updates/Dirty-Rectangles verwenden, wo anwendbar und für den UI-Stack korrekt.

## Review-Checkliste

- Controller/Schnittstelle/Pixelformat explizit identifiziert.
- Farbreihenfolge und Byte-Reihenfolge sind im Code/in der Konfiguration explizit.
- Flush-Puffer-Lebensdauer ist bis zum Abschluss der Transaktion gültig.
- DMA-/Speicherplatzierung erfüllt die Treiber-Anforderungen.
- Rotations-/Fenster-Mathematik passt zur Panel-Konfiguration.
- Performance-Tuning-Änderungen sind gemessen und bleiben visuell korrekt.
