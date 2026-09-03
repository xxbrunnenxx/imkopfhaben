# ESP-IDF-Checklisten

Nutze diese Checklisten, um Implementierung, Review und Debugging-Arbeit zu beschleunigen, ohne embedded-spezifische Risiken zu überspringen.

## Blockierender-Kontext-Checkliste (nicht überspringen)

- Exakten Ziel-Chip bestätigen (`esp32`, `esp32s3`, `esp32c3` usw.). Bei Unklarheit stoppen.
- Board/Revision und Peripherie-Verkabelung bestätigen (GPIO-Belegung, Pull-ups, Transceiver, Display-Interface).
- Elektrische Annahmen bestätigen (Spannungspegel, Power-Rails, Level-Shifting, gemeinsam genutzte Busse).
- ESP-IDF-Version und den vom Projekt genutzten Haupt-Treiber-API-Stil bestätigen.
- Alle genutzten Plugin-/Framework-Versionen bestätigen (ESP-ADF, ESP-SR usw.) und exakte Tags/Commits sammeln.
- Konkrete Kompatibilitätsnachweise für jedes Plugin/Framework gegen die gewählte ESP-IDF-Version bestätigen.
- Wenn mehrere Frameworks zusammenwirken (z. B. ESP-ADF + ESP-SR), explizite Cross-Stack-Kompatibilitätsnachweise bestätigen (nicht nur individuelle IDF-Kompatibilität).
- Bei unklaren Verhaltens- oder API-Nutzungserwartungen vor Implementierung/Debugging nach Beispielcode fragen (Projekt-Snippet, Hersteller-Beispiel oder minimaler Repro-Fall).
- Flash-Größe und ob OTA erforderlich ist bestätigen, bevor Partitions-Änderungen vorgeschlagen werden.
- PSRAM-Vorhandensein/-Modus bestätigen, falls Speicherplatzierung oder Display-Puffer betroffen sind.
- Display-/Controller-Modell und Pixelformat/Byte-Reihenfolge vor Grafik-Arbeit bestätigen.
- Bestätigen, ob ein USB-/Serial-Konsolenpfad für ein Service-Terminal verfügbar und frei ist (und ob Produkt-/Sicherheitsrichtlinie das erlaubt).
- Wenn eines der obigen Punkte bei hardware-nahen Änderungen unklar ist, den Nutzer fragen und die Implementierung/das Debugging nicht fortsetzen.

## Implementierungs-Checkliste

- Ziel-Chip (`esp32`, `esp32s3`, `esp32c3` usw.) und ESP-IDF-Version bestätigen.
- Bestätigen, dass die ESP-IDF-Toolchain installiert und nutzbar ist, bevor gebaut wird (`idf.py --version` läuft erfolgreich oder der Projekt-Wrapper-Preflight besteht).
- Bestätigen, dass der Plugin-/Framework-Kompatibilitäts-Preflight besteht und vor dem Bauen einen Nachweis-Report erzeugt.
- Board-seitige Pin-Belegung und elektrische Randbedingungen bestätigen, bevor GPIOs zugewiesen werden.
- Task-Modell bestätigen: Task-Prioritäten, Stack-Größen, Queue-Tiefen, Timer-Takt, Core-Zuordnung (falls genutzt).
- Eigentümerschaft von gemeinsam genutztem Zustand und Synchronisations-Primitiven definieren.
- ISR-Arbeit minimal halten und nur ISR-sichere APIs nutzen.
- `esp_err_t`-Rückgabewerte prüfen und Fehler explizit behandeln.
- Subsysteme in deterministischer Reihenfolge initialisieren (NVS, Netif/Event-Loop, WLAN/BLE, Treiber, App-Tasks).
- Logs für Zustandsübergänge, Wiederholversuche und Fehlerursachen ergänzen.
- Heap-Churn in Hot Paths vermeiden, wenn ein statischer Puffer oder ein Wiederverwendungs-Muster ausreicht.
- Wenn USB-/Serial-Transport frei und erlaubt ist, standardmäßig ein einfaches, nutzerfreundliches Service-Terminal (`esp_console`/REPL) mit Hilfe/Autovervollständigung und Kern-Diagnose-Kommandos ergänzen.
- `sdkconfig`/`sdkconfig.defaults` bewusst und reproduzierbar aktualisieren; Datei-Änderungen gegenüber Ad-hoc-`menuconfig`-Durchläufen bevorzugen.
- Partitionstabelle (`partitions.csv`) an Flash-Größe und Funktionsbedarf anpassen; unerklärten ungenutzten Flash vermeiden.
- Falls OTA erforderlich ist, OTA-kompatible Partitionen und ausreichende Slot-Größe prüfen.
- Bei Display-Pfaden Controller-Pixelformat, Farbreihenfolge und Puffer-Layout validieren, bevor Konvertierungen codiert werden.
- Nicht offensichtliche Timing-, Hardware- oder Protokoll-Annahmen dokumentieren.

## Code-Review-Checkliste

- Task-/ISR-Kontext-Korrektheit für jeden FreeRTOS- und ESP-IDF-API-Aufruf prüfen.
- Blockierende Aufrufe in hochprioren Tasks und Callbacks prüfen.
- Timeout-Werte auf Endlos-/Blockierverhalten prüfen, das den Fortschritt blockieren kann (Deadlock).
- Speicher-Eigentümerschaft und Lebensdauer von über Tasks/Callbacks weitergereichten Puffern prüfen.
- Event-Handler-Registrierung/-Deregistrierung und Risiken doppelter Registrierung prüfen.
- Fehler-Weitergabe und Aufräumen bei teilweisem Init-Fehlschlag prüfen.
- Watchdog-Exposition prüfen (lange kritische Abschnitte, Busy-Loops, deaktivierte Yields).
- Pin-/Peripherie-Konflikte und versteckte Annahmen in `sdkconfig` prüfen.
- Partitionstabelle und `sdkconfig` auf Übereinstimmung mit Flash-Größe, OTA-Anforderung und aktivierten Funktionen prüfen.
- Prüfen, ob Plugin-/Framework-Versionen fixiert/dokumentiert sind und zu bekannt-guten Kompatibilitätsnachweisen passen.
- Logging-Konfiguration prüfen: laute Bibliotheks-Logs bei Bedarf unterdrücken, während Anwendungs-Logs ausreichend ausführlich bleiben.
- Prüfen, ob ein freier USB-/Serial-Pfad ein Service-Terminal haben sollte und ob eines ohne Grund weggelassen wurde.
- Terminal-UX (Hilfe/Autovervollständigung/klare Fehler) und Kommando-Sicherheit prüfen, falls eine Konsole vorhanden ist.
- Grafik-/Display-Code auf explizite Format-Annahmen prüfen (RGB565/BGR565/RGB888 usw., Byte-Reihenfolge, Stride).
- Bus-/Peripherie-Takt, DMA und Speicherplatzierungs-Entscheidungen gegen Performance-Anforderungen prüfen.
- Log-Qualität für Feld-Diagnose prüfen (Tag, Event, Fehlercode, Zustand).

## Debugging-Checkliste

- Mit exakter Firmware-Revision, `sdkconfig`, Ziel und Hardware-Aufbau reproduzieren.
- Vollständiges Serial-Log vom Boot bis zum Fehlschlag erfassen.
- Fehlschlags-Phase klassifizieren: Build, Flash, Boot, Init, Laufzeit, Schlaf/Aufwachen, Netzwerk, Peripherie-I/O.
- Erstes schlechtes Symptom und das unmittelbar davor liegende Ereignis identifizieren.
- Gezielte Instrumentierung (Zähler, Zeitstempel, Zustands-Logs) vor dem Refactoring ergänzen.
- Variablen reduzieren: unbeteiligte Funktionen deaktivieren, Eingaben mocken oder ein einzelnes Subsystem isolieren.
- Strom-, Reset- und Verkabelungs-Annahmen für hardware-nahe Fehler validieren.
- Bei mehrdeutigen Symptomen und unzureichenden Belegen nach einem minimalen reproduzierbaren Beispiel oder bekannt-gutem Referenzcode fragen.
- Bei Display-Korruption/Farbproblemen Pixelformat/Byte-Reihenfolge/Controller-Init-Sequenz prüfen, bevor die App-Grafiklogik geändert wird.
- Bei lautem Build-Output die Komponenten-Log-Level so einstellen, dass Signal sichtbar bleibt, während App-Logs wertvoll bleiben.
- Falls ein Service-Terminal existiert, vor invasiven Code-Änderungen Laufzeit-Kommandos für Heap-/Task-/Log-Level-Introspektion nutzen.
- Nach jeder Änderung erneut testen; unabhängige Fixes nicht bündeln.

## Build-/Validierungs-Checkliste

- Falls vorhanden, den projekteigenen `build.sh`-Wrapper bevorzugen; sonst `idf.py build` nutzen.
- Vor dem Build-Lauf bestätigen, dass das ESP-IDF-Umgebungs-Setup gültig ist (`idf.py` läuft, nicht nur existiert).
- Vor dem Build-Lauf bestätigen, dass der Plugin-/Framework-Kompatibilitätsnachweis (Matrix/Manifest/Release-Notes-Beleg) konkret und für die exakt genutzten Versionen aktuell ist.
- Bei schlechter Shell-UX für Entwickler ein Shell-Helfer-Snippet ergänzen/aktualisieren (z. B. `.zshrc`) für `idf`-Env-Sourcing und PATH-Setup.
- Nach Code-/Konfig-/Partitions-Änderungen den Build ausführen, bevor Fertigstellung erklärt wird.
- Bei Build-Fehlschlag fixen und erneut ausführen, bis er besteht.
- Warnungen durchsehen; Korrektheits-/Sicherheits-Warnungen beheben statt ignorieren.
- Falls Warnungen bestehen bleiben, sie explizit mit Begründung und Auswirkung benennen.

## WLAN-/BLE-Fokus-Prüfungen

- Init-Reihenfolge bestätigen (`nvs_flash_init`, Event-Loop-/Netif-Setup, Stack-Init, Handler, Start/Verbinden).
- Reconnect-Strategie und Retry-/Backoff-Verhalten bestätigen.
- Zugangsdaten und persistenten Konfigurations-Zustand bestätigen (NVS).
- Event-Handling-Abdeckung für Trennungs-/Fehler-Events prüfen.
- Koexistenz-Annahmen prüfen, wenn WLAN und BLE zusammen laufen.

## Peripherie-Fokus-Prüfungen

- Spannungspegel, Pull-ups und gemeinsame Bus-Verkabelung bestätigen.
- Pin-Mux und etwaige Strapping-Pin-Einschränkungen bestätigen.
- Bus-Geschwindigkeit/Timing und geräte-spezifische Protokoll-Verzögerungen bestätigen.
- Transaktions-Timeouts und Erholung von Bus-Blockaden bestätigen.
- ISR-Zuordnung und DMA-Randbedingungen bestätigen, falls relevant.
- Flash-/PSRAM-Geschwindigkeits-/Modus-Annahmen bestätigen und die performanteste zuverlässige, von Hardware/Projekt unterstützte Konfiguration wählen.
