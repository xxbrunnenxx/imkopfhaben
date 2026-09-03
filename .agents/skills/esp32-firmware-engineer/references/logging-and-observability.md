# ESP32-Logging und Observability (ESP-IDF)

Diese Referenz nutzen beim Entwerfen von Logs, Filtern von Rauschen oder Diagnostizieren von Problemen über `idf.py monitor`.

## Logging-Richtlinie

- Anwendungs-Logs während der Entwicklung/Fehlersuche ausführlich und mit hohem Signal halten.
- Irrelevante Library-/Standard-Komponenten-Logs reduzieren, wenn sie die Zustandsübergänge der Anwendung verdecken.
- Gezieltes Filtern/Feinjustieren gegenüber globaler Unterdrückung bevorzugen.

## Praktische ESP-IDF-Logging-Hinweise

- Stabile Modul-Tags verwenden (`wifi_mgr`, `sensor_task`, `display_drv`, `ota_updater`).
- Loggen:
  - Zustandsübergänge
  - Fehlercodes (`esp_err_t`)
  - Retry-Anzahl/Backoff
  - Timing/Latenz (wenn relevant)
  - wichtige Konfigurationsentscheidungen beim Start
- Wiederholte, unstrukturierte Info-Logs in engen Schleifen vermeiden.
- Falls ein On-Device-Terminal vorhanden ist, Laufzeit-Log-Level-Steuerung (nach Tag/Wildcard) bereitstellen, damit das Signal ohne erneutes Flashen justiert werden kann.

## Strategie zur Rauschreduzierung

- Ausführlichkeit lauter Komponenten-Logs selektiv senken (Build-Zeit-Konfiguration oder Laufzeit-Log-Level-Steuerung, wo genutzt).
- App-Module bei `DEBUG`/`VERBOSE` belassen, während Drittanbieter-/Standard-Rauschen bei Bedarf reduziert wird.
- Genug System-Logs erhalten, um Reset-/Panic-/Netzwerk-Ereignisse diagnostizieren zu können.

## Wie gute Logs aussehen

- Ereignis-first und zustandsbehaftet:
  - `wifi_mgr: disconnected reason=... retry=3 backoff_ms=2000`
  - `display_drv: flush region x=0 y=0 w=240 h=40 fmt=rgb565`
- Kennungen für Peripherie/Geräte/Busse einschließen, wenn mehrere Instanzen existieren.
- Dauern für Timeouts und Retries einschließen.

## Review-Checkliste

- Anwendungs-Logs sind ausführlich genug, um das Verhalten zu debuggen.
- Library-/Standard-Rauschen ist reduziert, wenn es das Signal verdeckt.
- Terminal-Log-Level-Kommandos (falls vorhanden) sind eingegrenzt und sicher.
- Fehler-Logs enthalten Code + Kontext, nicht nur generischen Fehlertext.
- Start-Logs erfassen wichtige Ziel-/Konfigurationsannahmen.
- Logs verursachen keine übermäßige Timing-Störung in Hot Paths.
