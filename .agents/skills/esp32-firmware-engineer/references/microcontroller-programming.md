# ESP32-Peripherie-Programmierung (ESP-IDF)

Diese Referenz für ESP32-GPIO, Interrupts, Timer, ADC, PWM, Watchdogs und Low-Level-Programmierentscheidungen in ESP-IDF nutzen.

## ESP32-spezifischer Standard

- Zuerst ESP-IDF-Treiber und HAL-artige APIs bevorzugen.
- Direkte Register-Programmierung vermeiden, außer:
  - das Projekt macht es bereits so
  - eine benötigte Funktion ist im Treiber nicht verfügbar
  - es gibt einen gemessenen Performance-/Timing-Grund
- Werden direkte Register verwendet, hinter einer Komponenten-API isolieren und Chip-Annahmen dokumentieren.

## GPIO-Konfiguration

- `gpio_config()` mit explizitem Modus, Pull und Interrupt-Einstellungen verwenden.
- Pin-Fähigkeit auf dem gewählten Ziel validieren (Input-only-Pins, analogfähige Pins, Strapping-Pins, RTC-IO-Verfügbarkeit).
- Benannte Konstanten und Board-Definitionen gegenüber rohen, im Code verstreuten GPIO-Nummern bevorzugen.

Grundmuster:
- Board-Pin-Map-Header
- eine Init-Funktion pro Subsystem
- keine versteckte Rekonfiguration in fachfremden Modulen

## GPIO-Interrupts

- `gpio_install_isr_service()` und `gpio_isr_handler_add()` für GPIO-ISR-Verdrahtung verwenden.
- ISR-Handler mit `IRAM_ATTR` markieren, wenn vom konfigurierten Interrupt-Pfad gefordert.
- ISR-Handler minimal halten: Zeitstempel, Zustand latchen, Task benachrichtigen, zurückkehren.
- Entprellen im Task- oder Timer-Kontext, nicht durch Blockieren im ISR.

## Timer-Wahl (ESP-IDF)

- `esp_timer`: Software-Callbacks, hochauflösende Terminierung.
- `gptimer`: Hardware-Timer-/Capture-/Compare-Anwendungsfälle.
- FreeRTOS-Timer: unpräzises App-Level-Timing/Retries.

Nicht einen Timer-Typ für alle Probleme erzwingen. Nach Präzision, Callback-Kontext und CPU-Last auswählen.

## PWM und Puls-Ausgabe

- LEDC für gängige PWM-Anwendungsfälle bevorzugen (LED-Dimmen, einfache PWM-Ausgänge).
- MCPWM für motorsteuerungsartige Anforderungen nutzen, wo relevant und unterstützt.
- Timer-Auflösungs-/Frequenz-Kompromisse explizit validieren.

## ADC-Muster

- ESP-IDF-ADC-Treiber (Oneshot/Continuous, versionsabhängige APIs) und Kalibrierungs-Helfer bevorzugen, wenn Spannungsgenauigkeit wichtig ist.
- Explizit bei Dämpfung, Abtastbedingungen und Kalibrierungsquelle sein.
- Nicht annehmen, dass Laborbank-Spannungswerte im Feld unter Last/Rauschen übereinstimmen.
- "Rohes Sensor-Lesen" von "Umrechnung in technische Einheiten" im Code trennen, für einfacheres Testen.

## UART-/Serial-Logging-Integration

- Anwendungsprotokoll-UART-Handling getrennt von Konsolen-/Log-UART-Annahmen halten.
- Bei Nutzung von `idf.py monitor` für Logs Baudrate und Port-Annahmen in Debug-Schritten dokumentieren.
- Log-Flutung in engen Schleifen vermeiden; das verzerrt Timing und kann Race-/Watchdog-Probleme verdecken.

## Watchdogs (praktisch)

- Task-Watchdogs und System-Watchdogs bewusst einsetzen; nicht deaktivieren, um Starvation-Probleme zu verdecken.
- Watchdogs im Owner-Task-/Main-Loop-Pfad füttern, nicht in beliebigen Hilfsfunktionen.
- Bei Watchdog-Resets prüfen:
  - blockierende Aufrufe
  - Deadlocks
  - ISR-Stürme
  - lange kritische Abschnitte
  - Log-Flutung/Busy-Waits

## Taktung und Timing (ESP32)

- Takt-/Frequenzverhalten wird größtenteils über ESP-IDF und `sdkconfig` konfiguriert; manuellen Clock-Tree-artigen Code aus anderen MCUs vermeiden.
- Für zeitkritischen Code tatsächliche Intervalle messen (`esp_timer_get_time()`, Zeitstempel, Oszilloskop/Logikanalysator) statt nominale Frequenz anzunehmen.
- Bei durchsatzkritischer Peripherie (Display/Speicher/Streaming) Flash-/PSRAM-Modus, Bus-Takt, DMA-Nutzung und Speicherplatzierung gemeinsam prüfen; die beste Option ist hardware- und board-abhängig.

## Querverweis Low-Power-Programmierung

- Für Sleep/Wakeup und Power-Strategie `references/power-optimization.md` lesen.
- Für Kommunikationsbus-Timing und DMA-Belange `references/communication-protocols.md` lesen.

## Review-Checkliste (zusammengeführt und ESP32-adaptiert)

- ESP-IDF-Treiber verwendet, außer es gibt eine begründete Low-Level-Ausnahme.
- Pin-Fähigkeiten und Strapping-Einschränkungen für den Ziel-Chip geprüft.
- GPIO-ISR-Handler sind minimal, ISR-sicher und, wo gefordert, IRAM-sicher.
- Korrektes Timer-Subsystem gewählt (`esp_timer`, `gptimer`, FreeRTOS-Timer, LEDC/MCPWM).
- ADC-Dämpfungs-/Kalibrierungs-Annahmen dokumentiert.
- Watchdog-Handling bewahrt Diagnostik statt Probleme zu verdecken.
- Zeitkritisches Verhalten durch Messung/Logging validiert, nicht durch Annahmen.
