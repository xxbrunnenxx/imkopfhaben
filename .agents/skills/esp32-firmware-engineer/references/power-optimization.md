# ESP32-Power-Optimierung (ESP-IDF)

Diese Referenz für ESP32-Low-Power-Modi, Wakeup-Design, dynamisches Power-Management und akku-bewusstes Verhalten in ESP-IDF-Projekten nutzen.

## Erst mit Power-Budget und Wakeup-Modell beginnen

- Zuerst Ziel-Durchschnittsstrom, Active-Duty-Cycle, Wakeup-Quellen und Latenz-Anforderungen definieren.
- Power-Tuning ohne Messplan liefert meist irreführende Ergebnisse.
- Identifizieren, ob das Produkt ist:
  - dauerhaft verbunden, netzbetrieben
  - akkubetriebener periodischer Sensor
  - schubweises Funkgerät
  - latenzarmes interaktives Gerät

## ESP32-Sleep-Modi (praktisch)

- Active-Modus: CPU/Peripherie/Funkmodule laufen.
- Modem-Sleep: CPU aktiv, Funkmodul im Duty-Cycle-/Power-Save-Verhalten (abhängig vom WLAN-/BLE-Anwendungsfall).
- Light-Sleep: CPU pausiert mit schnellerem Aufwachen als Deep-Sleep; RAM bleibt erhalten (chip-/konfigurationsabhängig).
- Deep-Sleep: niedrigster gängiger Power-Modus; die meisten Laufzeitzustände gehen verloren, außer RTC-gehaltene Daten/konfigurierte Wake-Quellen.

Auswahl basierend auf:
- benötigter Wake-Latenz
- Anforderungen an Zustandserhalt
- Funk-Reconnect-Kosten
- Abtastintervall

## Wakeup-Quellen (ESP-IDF)

- Timer-Wakeup: `esp_sleep_enable_timer_wakeup(...)`
- GPIO-/EXT-Wakeup (chip-spezifische APIs und Einschränkungen unterscheiden sich je Ziel)
- ULP-/Coprozessor-Wakeup auf unterstützten Chips
- Touch-/UART-Wakeup auf unterstützten Zielen und Konfigurationen

Immer zielspezifische Wakeup-Unterstützung für den exakten Chip prüfen (`esp32`, `esp32s3`, `esp32c3` usw.).

## Dynamic Frequency Scaling und PM-Locks

- ESP-IDF-Power-Management-APIs gegenüber manueller Takt-Manipulation bevorzugen.
- `esp_pm_configure(...)` für DFS-/Light-Sleep-Policies verwenden, wo unterstützt.
- PM-Locks (`esp_pm_lock_*`) nur um Operationen herum verwenden, die wirklich eine Mindestfrequenz oder kein Light-Sleep benötigen.
- Locks prompt freigeben; ausgelaufene PM-Locks sind ein häufiger Grund, warum "Power Save nicht funktioniert".

## WLAN-/BLE-Power-Strategie

- Funkverhalten dominiert oft den Stromverbrauch.
- Auf Systemebene optimieren:
  - Netzwerkaktivität bündeln
  - Reconnect-Churn reduzieren
  - passenden WLAN-Power-Save-Modus nutzen
  - unnötige Scans/Advertising-Aktivität minimieren
- Power-Auswirkung von Retry-Schleifen und Fehlerbehandlung validieren; "schneller erholen" kann deutlich mehr Energie kosten.

## Peripherie-Power-Management

- Ungenutzte Peripherie/Treiber im Leerlauf deinitialisieren oder stoppen (ADC, SPI-Geräte, Sensoren, UARTs, falls sicher).
- Externe Sensoren/Versorgungsschienen mit Load-Switches abschalten, wo Hardware es erlaubt.
- Periodisches Polling vermeiden, wenn Interrupt-/Event-basiertes Wakeup machbar ist.
- DMA-/gequeute Transfers nutzen, um die CPU-Wachzeit bei Bulk-I/O zu reduzieren.

## GPIO-Leckage und Deep-Sleep-Überlegungen

- Ungenutzte Pins auf bekannte, für das Board-Design sichere Zustände konfigurieren (High-Z, Pull, oder getriebener Pegel, je nach Leckage-Pfad).
- Board-spezifische Leckage über externe Pull-ups, Level-Shifter, Sensoren und Transistor-Netzwerke prüfen.
- RTC-IO-Hold-/Isolation-Funktionen nutzen, wo passend und unterstützt.
- Auf Strapping-Pins und Boot-Anforderungen achten, wenn Standard-Pin-Zustände geändert werden.

## Akku-Überwachung und adaptives Verhalten

- Kalibrierte ADC-Messungen (ESP-IDF-ADC-Kalibrierungs-APIs) verwenden, wenn Spannungsgenauigkeit wichtig ist.
- Akku zu kontrollierten Zeitpunkten abtasten (Lastzustand beeinflusst die Spannung).
- Schwellenwerte mit Hysterese definieren, um Oszillation zu vermeiden.
- Arbeitslast anpassen:
  - seltener abtasten
  - Funkaktivität reduzieren
  - nicht-kritische Funktionen aufschieben

## Messung und Verifikation

- Strom mit geeigneten Werkzeugen messen (Power-Analyzer/Strommessgerät), nicht nur mit Software-Schätzungen.
- Vergleichen:
  - Idle Active
  - Light-Sleep
  - Deep-Sleep
  - Funk-TX/RX-Spitzen
  - Reconnect-Stürme/Fehlerbedingungen
- Exakte Firmware-Konfiguration (`sdkconfig`, Ziel, Board-Revision) mit den Messungen festhalten.

## Review-Checkliste (zusammengeführt und ESP32-adaptiert)

- Sleep-Modus basierend auf Latenz + Zustandserhalt + Reconnect-Kosten gewählt.
- Wakeup-Quellen und chip-spezifische Einschränkungen geprüft.
- PM-Locks nur bei Bedarf erworben und korrekt freigegeben.
- Funk-Retry-/Connect-Logik auf Energieauswirkung geprüft.
- Peripherie-/Sensor-Idle-Zustände und externe Versorgungsschienen-Steuerung berücksichtigt.
- GPIO-Leckage-Pfade und Strapping-Pin-Zustände geprüft.
- Akku-Schwellenwerte nutzen bei Bedarf Hysterese und kalibrierten ADC-Pfad.
- Power-Behauptungen durch Messung belegt, nicht nur durch Schätzungen.
