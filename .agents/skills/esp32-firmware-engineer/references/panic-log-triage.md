# Panic- und Log-Triage

Diese Datei nutzen beim Diagnostizieren von Resets, Panics, Boot-Loops und unklaren Laufzeit-Fehlern aus seriellen Logs.

## Zuerst die richtigen Daten sammeln

- Das vollständige serielle Log vom Reset bis zum Fehler erfassen (nicht nur das Panic-Ende).
- Exakte ESP-IDF-Version, Ziel-Chip und Build-Typ notieren.
- Das verwendete Kommando notieren (`idf.py monitor`, Baudrate, serieller Port).
- Die ELF-Datei speichern, die zur geflashten Binary passt, für die Symbol-Auflösung.

## Reset-/Fehler-Kategorien

- Build-/Link-Fehler: Compiler, Linker, Komponenten-Abhängigkeit oder Konfigurations-Mismatch.
- Flash-/Verbindungs-Fehler: serieller Port, Berechtigungen, Kabel, Boot-Modus, Stub-/Baudrate-Probleme.
- Boot-Fehler: Partitionstabelle, Image-Mismatch, früher Init-Crash, fehlende Konfiguration/Daten.
- Laufzeit-Panic: Null-Dereferenzierung, Stack-Overflow, illegale Instruktion, Watchdog-Timeout.
- Funktionaler Laufzeit-Fehler: kein Panic, aber fehlerhaftes Verhalten, Timeouts oder verlorene Konnektivität.

## Panic-Triage-Ablauf

1. Reset-Grund / Panic-Überschrift identifizieren.
2. Die Zeilen unmittelbar vor dem Panic lesen, um das auslösende Subsystem zu finden.
3. Backtrace gegen die passende ELF dekodieren (Monitor-Dekodierung oder addr2line-Workflow nutzen).
4. Die obersten Frames und den ersten App-Frame untersuchen.
5. Kürzliche Änderungen an diesem Subsystem, Task, Puffer oder Callback-Pfad prüfen.
6. Gezielte Logs/Asserts rund um die vermutete Grenze ergänzen.

## Häufige Embedded-Grundursachen zum Prüfen

- Null-/uninitialisierte Handles nach teilweisem Init-Fehlschlag.
- Stack-Overflow in einem Task mit Logging, JSON-Parsing, TLS- oder BLE-/WLAN-Callbacks.
- Use-after-free oder Puffer-Lebensdauer, die Task-Grenzen überschreitet.
- Aufruf nicht-ISR-sicherer APIs aus einem ISR- oder Callback-Kontext.
- Race Conditions um gemeinsam genutzte Flags/Queues ohne Synchronisation.
- Watchdog durch blockierende Schleife, Deadlock oder lange kritische Sektion.
- Falsch konfigurierte Pins/Peripherie, die Treiber-Timeouts verursachen, die in Watchdog-Resets kaskadieren.

## Logging-Hinweise

- Stabile Log-Tags pro Modul verwenden (`wifi_mgr`, `sensor_task`, `ble_gatt` usw.).
- Zustandsübergänge und Fehlercodes loggen, nicht nur generischen Fehlertext.
- Retry-Anzahl und Timeout-Dauern einschließen, wenn Reconnect-Schleifen diagnostiziert werden.
- Temporäre High-Signal-Logs ergänzen, nach dem Fix entfernen oder herabstufen.

## Nützliche Kommandos (an das Projekt anpassen)

- `idf.py build`
- `idf.py flash monitor`
- `idf.py fullclean build`
- `idf.py menuconfig`

Hinweis: versionsspezifische Ausgabe und Panic-Formatierung können sich zwischen ESP-IDF-Releases unterscheiden. Logs bevorzugt mit der tatsächlichen ESP-IDF-Version des Projekts und passenden ELF-Artefakten interpretieren.
