# ESP32-Geräte-Terminal / Service-Konsole (ESP-IDF)

Nutze diese Referenz, wenn ein USB-/Serial-Konsolenpfad verfügbar und nicht durch Produktfunktionalität belegt ist. Standardverhalten: proaktiv ein einfaches On-Device-Terminal für Serviceability, Beobachtbarkeit und Tuning ergänzen.

## Wann es ergänzt werden soll (Standard-Richtlinie)

- Standardmäßig ein Terminal ergänzen, wenn:
  - ein USB-/Serial-Transport verfügbar ist (`USB CDC`, `USB-Serial-JTAG` oder Board-USB-UART)
  - er nicht schon einer anderen Produktfunktion/einem Protokoll fest zugeordnet ist
  - Projekt-/Sicherheitsanforderungen eine interaktive Konsole nicht verbieten
- Nicht darauf warten, dass der Nutzer explizit danach fragt, wenn die obigen Bedingungen erfüllt sind.

## Transport-Auswahl (ESP32-Variante beachten)

- `ESP32-S2/S3`: native USB-Device-Optionen (oft USB CDC) können je nach Board-Design und Stack-Nutzung verfügbar sein.
- `ESP32-C3/C6/S3`: `USB-Serial-JTAG` kann verfügbar und praktisch für Service-Konsole/Monitor-Abläufe sein.
- `ESP32` (klassisch): nutzt oft eine externe USB-UART-Brücke zur UART-Konsole.

Vor der Implementierung bestätigen:
- welcher Transport physisch mit dem Host verbunden ist
- ob der Transport bereits für ein anderes Laufzeit-Protokoll genutzt wird
- ob JTAG-/Debug-Zugriff erhalten bleiben muss

## Bevorzugter Implementierungs-Ansatz

- ESP-IDF-Konsolen-Primitive (`esp_console`) und REPL-Helfer gegenüber eigenen Kommando-Parsern bevorzugen.
- Eingebaute Zeilenbearbeitungs-/Verlaufs-/Vervollständigungs-Unterstützung bevorzugen (linenoise-gestützte REPL in ESP-IDF).
- Kommandos in einer kleinen Kommando-Registry registrieren statt einen monolithischen `if/else`-Parser zu schreiben.
- Kommando-Handler schnell und deterministisch halten; lange Operationen bei Bedarf an Tasks auslagern.
- `assets/templates/esp-console/` als Ausgangspunkt für Kommando-Registrierung wiederverwenden.

Warum:
- bessere UX (Verlauf, Vervollständigung, Hilfe)
- konsistentes Kommando-Parsing
- einfachere Erweiterung und Review

## UX-Anforderungen (nutzerfreundlich)

- Autovervollständigung für Kommandonamen (und wo praktikabel für zentrale Subkommandos)
- `help`-Kommando mit kurzen Beschreibungen
- klare Fehlermeldungen und Nutzungshinweise
- stabile Kommando-Benennung (`settings`, `status`, `tasks`, `heap`, `log`, `reboot`)
- konsistentes Ausgabeformat (zuerst menschenlesbar, optional skript-freundlich)

## Minimal sinnvolles Kommando-Set (empfohlen)

- `help`: Kommandos und Nutzung auflisten
- `status`: Betriebsdauer, Firmware-Version, Ziel, Reset-Grund, Verbindungszustand
- `settings get <key>` / `settings set <key> <value>`: Anwendungs-Einstellungen (mit Validierung)
- `settings save` / `settings load` (falls Einstellungen nicht automatisch persistiert werden)
- `tasks`: RTOS-Task-Liste/-Zustände/-Stack-Hochwassermarken (build-konfigurationsabhängig)
- `heap`: frei/minimal/größter Block (capability-spezifische Varianten, falls nützlich)
- `log level <tag|*> <level>`: Laufzeit-Log-Tuning für laute vs. App-Komponenten
- `reboot`: kontrollierter Neustart (mit Bestätigungsoption für Produktions-Tools)

Optionale, wertvolle Kommandos:
- `wifi status` / `wifi reconnect`
- `i2c scan` (Vorsicht: nur wenn auf ausgeliefertem Gerät sicher)
- `display test` (formatvalidierte Muster)
- `nvs dump` / `nvs get` (keine Geheimnisse offenlegen)

## RTOS-Debug-Informationen (mit Vorsicht offenlegen)

- Leichtgewichtige Momentaufnahmen liefern, keine langen blockierenden Reports.
- Häufig nützliche Ausgaben:
  - Task-Name/-Zustand/-Priorität
  - Stack-Hochwassermarke
  - CPU-Nutzung/Laufzeit-Statistiken (falls konfiguriert)
- Manche erweiterten RTOS-Statistiken brauchen `sdkconfig`-Optionen (Trace-/Laufzeit-Statistik-Unterstützung). Bewusst bestätigen und aktivieren.
- Kommandos vermeiden, die das Timing in Produktion destabilisieren.

## Design der Einstellungs-Schnittstelle

- Werte vor dem Anwenden validieren.
- `set` von `save` trennen, wenn Persistenz-Nebeneffekte relevant sind.
- Exakte Validierungsfehler ausgeben (`out of range`, `unsupported enum`, `requires reboot`).
- Einstellungs-Änderungen mit Quelle (`terminal`) und Zeitstempel/Betriebsdauer loggen, falls verfügbar.
- Geheimnisse standardmäßig nicht im Klartext offenlegen.

## Logging-Integration

- Das Terminal soll Logs ergänzen, nicht ersetzen.
- Laufzeit-Log-Level-Kommandos ergänzen, um laute Komponenten leiser und App-Module lauter zu stellen.
- Terminal-Ausgabe knapp halten, um die Lesbarkeit des Monitors nicht zu stören.

## Sicherheits-/Produktions-Randbedingungen

- Falls das Produkt Sicherheitsanforderungen hat, sensible Kommandos absichern über:
  - Compile-Time-Feature-Flags
  - Build-Profil (dev vs. prod)
  - Authentifizierung/Challenge-Response (falls erforderlich)
- Destruktive Kommandos (`erase`, uneingeschränktes Speicher-Poke) deaktivieren oder einschränken, sofern nicht explizit erforderlich.

## Review-Checkliste

- Eigentümerschaft des Konsolen-Transports ist bestätigt (USB/CDC/JTAG/UART kollidieren nicht).
- `esp_console`/REPL wird statt eines Ad-hoc-Parsers genutzt (sofern nicht begründet anders).
- Autovervollständigung/Hilfe/Verlauf sind aktiviert und nutzbar.
- Kommando-Handler validieren Eingaben und melden handlungsfähige Fehler.
- RTOS-/Heap-Diagnostik ist begrenzt und sicher.
- Einstellungs-Kommandos schützen Geheimnisse, Persistenz-Verhalten ist explizit.
- Sicherheits-/Build-Profil-Absicherung ist wo nötig angewendet.
