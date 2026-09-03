# ESP32-Speicher- und Größen-Optimierung (ESP-IDF)

Diese Referenz für RAM-/Flash-/Code-Größen-Optimierung und Speichersicherheits-Entscheidungen in ESP-IDF-Projekten nutzen.

## ESP32-Speichermodell (praktische Sicht)

- Interner RAM ist begrenzt und wird mit Stacks, Treibern und Protokoll-Stacks geteilt.
- Manche Funktionen (WLAN/BLE, Netzwerk, TLS) erhöhen den internen RAM-Druck erheblich.
- PSRAM ist auf manchen Modulen/Zielen verfügbar, aber nicht aller Speicher ist gleich:
  - Latenz unterscheidet sich vom internen RAM
  - DMA-Kompatibilität ist eingeschränkt
  - Manche ISR-/kritischen Pfade sollten im internen Speicher bleiben
- ESP-IDF-Heap-Capabilities-APIs verwenden, wenn die Speicherklasse wichtig ist (`heap_caps_*`).
- Performance hängt von Speicherplatzierung und Bus-Modus ab, nicht nur von freien Bytes; interner RAM, PSRAM und Flash-basierter Code/Daten haben unterschiedliches Latenz-/Durchsatz-Verhalten.

## Allokationsrichtlinie (was zu bevorzugen ist)

- Statische Allokation für langlebige Puffer und Kern-Kontrollstrukturen bevorzugen.
- Arbeitspuffer in nicht überlappenden Pfaden wiederverwenden.
- Heap-Allokation in Hot Paths und Callback-lastigen Pfaden vermeiden.
- Nie aus ISR-Kontext allokieren.
- Feste Pools verwenden, wenn begrenztes dynamisches Verhalten benötigt wird.

## DMA-/Capability-bewusste Allokation

- DMA-Puffer bei Bedarf durch SPI-/I2S-/Peripherie-Treiber mit Capability-Flags allokieren (z. B. `MALLOC_CAP_DMA`).
- Puffer-Lebensdauer über asynchrone Transaktionen hinweg prüfen (gequeutes SPI/UART/etc.).
- Nicht annehmen, dass PSRAM-Puffer für jeden DMA-Pfad gültig sind.

## Stack-Dimensionierung und Überwachung

- Mit konservativen Task-Stacks für Parsing-/Logging-/Netzwerk-Code beginnen, dann messen und straffen.
- Stack-High-Water-Marks überwachen (`uxTaskGetStackHighWaterMark()` / `uxTaskGetStackHighWaterMark2()`, je nach Konfiguration/API-Verfügbarkeit).
- Auf verstecktes Stack-Wachstum achten durch:
  - Große lokale Arrays
  - Tiefe Aufrufketten
  - Logging und Format-Strings
  - JSON-/TLS-/Protokoll-Parser

## Heap-Überwachung und Fragmentierungs-Bewusstsein

- Verfolgen:
  - `heap_caps_get_free_size(...)`
  - `heap_caps_get_minimum_free_size(...)`
  - größter freier Block bei Fragmentierungsverdacht
- Vor/nach Feature-Init und im eingeschwungenen Laufzeitzustand messen.
- Wiederholtes Alloc/Free variabel großer Puffer ist eine häufige Fragmentierungsquelle.

## Code-Größen-Optimierung (ESP-IDF-Workflow)

- `idf.py size` und `idf.py size-components` nutzen, um Wachstum zu identifizieren.
- Linker-Map (`build/<app>.map`) inspizieren, wenn Komponenten-Level-Output nicht reicht.
- `const`-Daten für Nur-Lese-Tabellen und Strings bevorzugen.
- Komponenten-Abhängigkeiten minimal halten; ungenutzte Komponenten können überraschenden Code mitziehen.
- Logging-Levels und formatlastigen Debug-Code in Release-Builds überprüfen.
- Falls Ausführungsgeschwindigkeit wichtig ist, prüfen, ob Hot-Code-/Daten-Platzierung und Flash-/PSRAM-Konfiguration (`sdkconfig`) den Durchsatz begrenzen.

Übliche Stellschrauben (projektabhängig):
- `sdkconfig`-Optimierungsstufe (`CONFIG_COMPILER_OPTIMIZATION_*`)
- Link-Time-Optimierung (falls im Projekt-/Toolchain-Setup aktiviert/unterstützt)
- Reduzierung aktivierter Features/Komponenten/Protokolle

## Datenstruktur- und Puffer-Muster

- Den kleinsten Typ verwenden, der zu Protokollbereich und Alignment-Anforderungen passt.
- Strukturen nur packen, wenn Layout-Kompatibilität erforderlich ist (Protokoll-/Flash-Format); unnötige gepackte Structs in Hot Code wegen Alignment-Strafen vermeiden.
- Ringpuffer/Stream-Puffer für Byte-Streams bevorzugen.
- Explizite Ownership-Kommentare für Puffer verwenden, die Task-Grenzen überschreiten.

## Flash-/NVS-/Partitions-Überlegungen

- NVS für kleine persistente Konfiguration/Zustand statt Ad-hoc-Rohschreibzugriffen auf Flash verwenden.
- Für schreiblastige Anwendungsdaten eine verschleiß-bewusste Strategie entwerfen (NVS, Dateisystem, oder eigene Log-Struktur mit Rotation).
- Partitionstabelle und OTA-Slot-Größe im Blick behalten, wenn die Code-Größe wächst.
- Prüfen, ob große Assets/Tabellen überhaupt in die Firmware gehören; sie passen unter Umständen besser ins Dateisystem oder externen Speicher.

## Compile-Time-Guards

- `_Static_assert` / `static_assert` verwenden für:
  - Protokoll-Struct-Größen
  - Array-Längen
  - Queue-Payload-Größen
  - Compile-Zeit-Konfigurationsannahmen

## Review-Checkliste (zusammengeführt und ESP32-adaptiert)

- Statische/wiederverwendete Puffer gegenüber Ad-hoc-Heap-Allokationen bevorzugt.
- Keine Heap-Allokation in ISR- oder zeitkritischen Pfaden.
- DMA-Puffer nutzen bei Bedarf Capability-bewusste Allokation.
- Task-Stacks aus Messungen dimensioniert; High-Water-Marks geprüft.
- Heap-Minimum-frei und Fragmentierungs-Indikatoren in Tests überwacht.
- `const` für Nur-Lese-Daten verwendet.
- Code-Größen-Wachstum mit `idf.py size`/Komponenten-Aufschlüsselung geprüft.
- Partitions-/OTA-/NVS-Implikationen bei Flash-Nutzungsänderungen berücksichtigt.
- RAM-/Flash-/PSRAM-Konfiguration und Platzierungsentscheidungen für performance-kritische Pfade überprüft.
