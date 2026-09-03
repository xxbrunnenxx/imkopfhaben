# ESP32-Kommunikationsprotokolle (ESP-IDF)

Diese Referenz für ESP32-Peripherie-Kommunikationsmuster in ESP-IDF nutzen: I2C, SPI, UART und TWAI (CAN).

## Umfang und Versionshinweise

- ESP-IDF hat in manchen Subsystemen sowohl Legacy- als auch neuere Treiber-APIs (insbesondere I2C/ADC über Versionen hinweg).
- Den bestehenden API-Stil des Projekts bevorzugen, außer bei einer ausdrücklichen Migration.
- Vor dem Codieren immer Ziel-Chip und Pin-Map bestätigen (ESP32 vs. ESP32-S3/C3/C6-Funktionsunterschiede, Pin-Fähigkeiten, Strapping-Pins).

## I2C-(Master-)Muster

- Externe Pull-ups, Bus-Spannungskompatibilität und Bus-Geschwindigkeit prüfen, bevor Software debuggt wird.
- Immer Transaktions-Timeouts verwenden; nie unbegrenzt auf einen belegten Bus warten.
- Bus-Recovery/-Reset bei wiederholten Timeout-Bedingungen behandeln (Geräte-Lockups sind häufig).
- Zugriff mit einem Mutex oder einem dedizierten Bus-Owner-Task serialisieren.
- Adresse, Register, Timeout und Fehlercode für Feld-Diagnose loggen.

Übliche ESP-IDF-Muster:
- Legacy-API: `i2c_param_config()`, `i2c_driver_install()`, Command Links.
- Neuere API (IDF v5+): Bus-/Device-Handles mit expliziter Geräte-Konfiguration und Transfer-Timeout.

## SPI-(Master-)Muster

- `spi_bus_initialize()` und `spi_bus_add_device()` verwenden und die Geräte-Konfiguration (Modus, Takt, CS, Queue-Tiefe) explizit halten.
- Für DMA-Transfers bei Bedarf Puffer mit DMA-fähigem Speicher (`MALLOC_CAP_DMA`) allokieren.
- Bei mehreren Tasks, die sich den Bus teilen, die Transaktions-Eigentümerschaft explizit klären.
- Maximaltakt gegen Kabellänge, Signalintegrität und Geräte-Timing prüfen, nicht nur gegen die Datenblatt-Überschrift.
- Queued Transactions für Durchsatz bevorzugen; synchrones Senden für einfache Steuerpfade bevorzugen.

Review-Hinweise:
- Sind TX/RX-Puffer für die Dauer der Transaktion gültig?
- Ist das CS-Verhalten bei mehrteiligen Register-Operationen korrekt?
- Werden DMA-fähige Puffer verwendet, wo nötig?

## UART-Muster

- Den ESP-IDF-UART-Treiber (`uart_driver_install`) mit dem treibereigenen Ringpuffer/Event-Queue bevorzugen, bevor ein eigener ISR-Puffer geschrieben wird.
- Für gerahmte Protokolle einen dedizierten Parser-Task verwenden.
- Parser-Arbeit begrenzen und fehlerhafte Frames/Rauschen behandeln.
- Bei einem ISR-Callback-Pfad diesen minimal und bei Bedarf IRAM-sicher halten.

Typische Architektur:
- UART-Treiber-ISR/Ringpuffer -> Parser-Task -> Anwendungs-Queue/Zustandsautomat

## TWAI-(CAN-)Muster

- Auf ESP32-Chips mit TWAI-Unterstützung den TWAI-Treiber verwenden (`twai_driver_install`, Start/Stop/Transmit/Receive, Alerts).
- Zuerst Transceiver-Verkabelung und -Terminierung prüfen; Software wird oft fälschlich für elektrische Bus-Probleme verantwortlich gemacht.
- Alerts und Fehlerzähler nutzen, um Bus-off/Warning-Zustände von Anwendungsfehlern zu unterscheiden.
- Recovery-Logik für Bus-off implementieren, statt Transmit endlos zu wiederholen.

## RMT-/Sonderprotokoll-Hinweis

- Für zeitkritische One-Wire-/IR-/Puls-Protokolle RMT statt Bit-Banging in Tasks bevorzugen.
- RMT reduziert Jitter und CPU-Last oft im Vergleich zu Software-Timing-Schleifen.

## Design eines gemeinsam genutzten Kommunikationsbusses

- Einen Bus-Manager-Task bevorzugen, wenn:
  - Mehrere Tasks Transaktionen ausgeben
  - Reihenfolge wichtig ist (Sensor-Init + Reads + Kalibrierungs-Writes)
  - Retries/Recovery zentralisiert sein müssen
- Ein Mutex nur verwenden, wenn Transaktionen kurz sind und die Eigentümerschaft einfach ist.

## Fehlerbehandlung und Recovery (zusammengeführt und ESP32-adaptiert)

- Immer Timeouts verwenden, um Deadlocks/Stalls zu verhindern.
- `esp_err_t` (plus protokollspezifischen Status, falls verfügbar) weitergeben und loggen.
- Retry mit Backoff für transiente Fehler implementieren; enge Retry-Schleifen vermeiden.
- Hardware-Fehler (Verkabelung/Pull-up/Strom) von Protokoll-Framing-/Software-Fehlern unterscheiden.
- Empfangene Payloads validieren (CRC/Checksumme/Länge/Zustandsautomat-Übergänge).

## Hardware- und Pin-Einschränkungen (ESP32-spezifisch)

- GPIO-Matrix-Routing-Grenzen und Peripherie-Pin-Fähigkeit für den gewählten Chip prüfen.
- Strapping-Pins und Boot-Modus-Interaktionen beachten.
- Spannungspegel bestätigen (3,3V-Logik, Open-Drain-Pull-ups für I2C, Transceiver-Anforderungen für TWAI/RS-485).
- Bus-Geschwindigkeit/-Timing gegen Kabellänge und Pull-up-Stärke prüfen.

## Review-Checkliste

- Timeouts bei allen Protokoll-Operationen vorhanden.
- Gemeinsamer Bus-Zugriff korrekt serialisiert (Mutex oder Owner-Task).
- ISR-sichere APIs nur in ISR-Pfaden verwendet.
- DMA-fähige Puffer verwendet, wo nötig.
- Protokoll-Parsing validiert Länge/Zustand/CRC.
- Logs enthalten genug Kontext (Bus, Geräte-Adresse/-ID, Operation, Fehler, Timeout).
- Recovery-Pfad existiert für Bus-Lockups/Geräte-Reset/Bus-off.
