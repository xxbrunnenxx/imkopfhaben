# Gemini Service (auf diesem Branch abgelöst)

**Auf `folloup-waveshare` ist dieses Dokument historisch.** `components/gemini_service/`
wurde in `components/local_ai_service/` umbenannt und gegen einen lokalen
LM-Studio-Server statt die Gemini-Cloud-API neu implementiert -- siehe
[`docs/local-ai-service.md`](local-ai-service.md) für den aktuellen Stand. Diese Datei
bleibt unverändert erhalten, weil sie noch genau beschreibt, wie der Code vor
dieser Änderung aussah, und weil Upstream (`alxv2016/folloup-sticky`, andere
Branches dieses Forks) sie eventuell noch verwendet.

---

Dieses Dokument beschreibt die Gemini-Integration, die von früheren Revisionen
der Followup-Firmware auf diesem Branch und von anderen Branches dieses Forks
verwendet wurde.

`components/gemini_service/` verantwortet aktuell:

- Vorrangregelung der Gemini-API-Key-Speicherung
- Gemini-Authentifizierungsstatus
- Backend-HTTP-Routen für Gemini-Einstellungen und Laufzeitstatus
- WLAN-gesteuerte Gemini-Bereitschaft
- Gemini-Ready-Status, der von der Statusleiste konsumiert wird

Es verantwortet nicht:

- Transkriptions-Anfragen -- gehören der `transcription_service`-Komponente
- Zusammenfassungs-Erzeugung -- gehört der `summary_service`-Komponente
- Anreicherung/Indizierung archivierter Aufnahmen -- gehört
  `recording_archive_service`
- eine Frontend-Portal-UI

`transcription_service` und `summary_service` rufen die Gemini-API direkt auf;
sie wurden in eigene Komponenten ausgelagert statt innerhalb von
`gemini_service` zu leben, das sich weiterhin auf Key-/Einstellungs-Vorrang,
Auth-Bereitschaft und Status konzentriert.

## Zuständigkeiten

Aktuelle Laufzeit-Aufteilung:

- `gemini_service`
  - Gemini-Konfiguration, API-Key-Vorrang, Auth-Anfragen, Auth-Status und
    Backend-Route-Handler
- `wifi_service`
  - verantwortet den Backend-HTTP-Server und hostet Gemini-Routen über die
    bestehende Portal-Routen-Registrierung
- `main/app_shell.cpp`
  - initialisiert den Service, leitet WLAN-Netzwerkstatus an Gemini weiter,
    protokolliert Gemini-Ereignisse und löst den Gemini-verbunden-Sound-Cue aus
- `main/status_bar_runtime.cpp`
  - spiegelt den Gemini-Ready-Status in `epaper_ui::StatusBarState`
- `components/epaper_ui/`
  - rendert das Gemini-Ready-Stern-Symbol in der Statusleiste
- `feedback_service` / `system_sound_service`
  - spielen einen eigenen Gemini-verbunden-Cue ab, wenn die Bereitschaft von
    false auf true wechselt

`app_shell` bleibt hier ein Orchestrator. Der Gemini-Service verantwortet den
Provider-Status und das Routen-Verhalten; `app_shell` verdrahtet nur
Ereignisse, Startreihenfolge und produktseitige Reaktionen.

## Interner Aufbau

Followup nutzt aktuell eine Gemini-Service-Implementierung in einer einzigen
Datei:

- [`components/gemini_service/include/gemini_service.h`](/Users/tieuvong/Development/folloup-sticky/components/gemini_service/include/gemini_service.h)
- [`components/gemini_service/gemini_service.cpp`](/Users/tieuvong/Development/folloup-sticky/components/gemini_service/gemini_service.cpp)

Aktuelle interne Zuständigkeiten innerhalb dieser Komponente:

- NVS-Lesen/Schreiben des gespeicherten Gemini-API-Keys
- sdkconfig-Fallback-Key-Lookup
- Auflösung der effektiven Key-Vorrangregelung
- HTTP-`GET`-Modell-Authentifizierung gegen Gemini
- Auth-Task-Lebenszyklus und Schutz vor veralteten Ergebnissen
- Backend-JSON-Anfrage-/Antwort-Verarbeitung

Followup teilt das noch nicht in separate `client`-, `worker`- und
`settings_storage`-Dateien auf, wie es Followup sonst tut.

## API-Key-Quellen

Aktuelle API-Key-Vorrangregelung:

1. per Backend-API gespeicherter NVS-Key
2. eingebauter `CONFIG_FOLLOWUP_GEMINI_API_KEY`
3. kein Key konfiguriert

Ein Zurücksetzen der Gemini-Einstellungen löscht nur den gespeicherten
NVS-Key. Der eingebaute sdkconfig-Fallback-Key wird dabei nicht gelöscht.

## Build-Zeit-Konfiguration

Die Build-Zeit-Gemini-Einstellung liegt unter `Folloup Settings`:

- `CONFIG_FOLLOWUP_GEMINI_API_KEY`

Das ist für Entwicklung und Testaufbauten gedacht. Ein per Backend-API
gespeicherter Key hat Vorrang vor dem eingebauten Key.

Der reproduzierbare Standardwert steht in:

- [`sdkconfig.defaults`](/Users/tieuvong/Development/folloup-sticky/sdkconfig.defaults)

Aktueller Standardwert:

- `CONFIG_FOLLOWUP_GEMINI_API_KEY=""`

## Start- und Bereitschafts-Ablauf

Aktueller Auth-Ablauf:

1. `app_shell` initialisiert `gemini_service`
2. der Service lädt den gespeicherten NVS-Key, falls vorhanden
3. `wifi_service` erreicht einen verbundenen STA-Zustand
4. `app_shell` leitet den Netzwerkstatus an
   `gemini_service::SetNetworkState(...)` weiter
5. wenn ein Gemini-API-Key verfügbar ist und noch keine Auth-Anfrage läuft,
   startet Gemini die Authentifizierung automatisch
6. Gemini führt ein Modell-`GET` gegen
   `https://generativelanguage.googleapis.com/v1beta/` aus
7. erfolgreiche Auth markiert den Runtime-Snapshot als bereit
8. die Statusleiste zeigt das Gemini-Ready-Stern-Symbol
9. die Feedback-Ebene spielt beim ersten Wechsel der Bereitschaft auf true
   den Gemini-verbunden-Cue

Die Authentifizierung wird übersprungen, wenn:

- kein API-Key konfiguriert ist
- bereits eine Anfrage läuft
- kein WLAN verbunden ist
- WLAN im Access-Point-Modus ist

Followup behandelt `ready` aktuell als:

- `configured == true`
- `authenticated == true`

## Transport-Details

Der aktuelle Auth-Pfad ist bewusst minimal:

- Anfrage-Typ: HTTP `GET`
- Endpunkt: `v1beta/<model>`
- Standard-Modell: `models/gemini-2.5-flash-lite`
- Auth-Header: `x-goog-api-key: <api_key>`

Die aktuelle Implementierung nutzt ESP-IDFs `esp_http_client` mit dem
CRT-Bundle zur TLS-Validierung.

Followup implementiert noch nicht:

- Gemini-Datei-Upload
- Transkriptions-Prompts
- Text-Erzeugung
- Token-Zählung

## Öffentliche C++-Snapshot-Formen

Quelle der Wahrheit:

- [`components/gemini_service/include/gemini_service.h`](/Users/tieuvong/Development/folloup-sticky/components/gemini_service/include/gemini_service.h)

### `gemini_service::SettingsSnapshot`

Aktuelle Felder:

- `configured`
- `has_stored_api_key`
- `has_sdkconfig_api_key`
- `api_key_source`
- `api_key_last4`
- `model_name`

### `gemini_service::RuntimeSnapshot`

Aktuelle Felder:

- `initialized`
- `ready`
- `request_in_flight`
- `auth_checked`
- `authenticated`
- `supports_audio_understanding`
- `supports_structured_output`
- `last_http_status`
- `last_status_message`
- `last_model_resource_name`
- `last_model_display_name`
- `last_error_code`
- `last_error_message`

`supports_audio_understanding` und `supports_structured_output` bleiben
aktuell `false`, weil diese Firmware diese Provider-Fähigkeiten noch nicht
portiert hat.

### `gemini_service::Snapshot`

Enthält:

- `settings`
- `runtime`

### `gemini_service::SettingsPatch`

Aktuelle Felder:

- `has_api_key`
- `api_key`

### `gemini_service::Result`

Aktuelle Felder:

- `success`
- `validation_error`
- `status_code`
- `field`
- `error_code`
- `message`

## Backend-Endpunkte

Aktuelle Backend-Endpunkte:

- `GET /api/settings/gemini`
- `PATCH /api/settings/gemini`
- `POST /api/settings/gemini/reset`
- `GET /api/runtime/gemini`

Diese Routen werden über den bestehenden WLAN-Backend-Server registriert.
Gemini startet keinen eigenen HTTP-Server.

Einschränkungen:

- Content-Type ist JSON
- `PATCH` erfordert einen JSON-Objekt-Body
- der `PATCH`-Body muss größer als `0` Byte und höchstens `512` Byte sein

## JSON-Verträge

### `PATCH /api/settings/gemini`

Aktuelle Anfrage-Form:

```json
{
  "api_key": "AIza..."
}
```

Aktuelle Validierungsregeln:

- `api_key` muss vorhanden sein
- `api_key` muss nach dem Trimmen eine nicht-leere Zeichenkette sein

### Snapshot-Antworten

Verwendet von:

- `GET /api/settings/gemini`
- `GET /api/runtime/gemini`
- erfolgreichem `PATCH /api/settings/gemini`
- erfolgreichem `POST /api/settings/gemini/reset`

Aktuelle Antwort-Form:

```json
{
  "success": true,
  "message": "Gemini settings loaded",
  "settings": {
    "configured": true,
    "has_stored_api_key": false,
    "has_sdkconfig_api_key": true,
    "api_key_source": "sdkconfig",
    "api_key_last4": "1234",
    "model_name": "models/gemini-2.5-flash-lite"
  },
  "runtime": {
    "initialized": true,
    "ready": true,
    "request_in_flight": false,
    "auth_checked": true,
    "authenticated": true,
    "supports_audio_understanding": false,
    "supports_structured_output": false,
    "last_http_status": 200,
    "last_status_message": "Authenticated with Gemini 2.5 Flash-Lite",
    "last_model_resource_name": "models/gemini-2.5-flash-lite",
    "last_model_display_name": "Gemini 2.5 Flash-Lite",
    "last_error_code": "",
    "last_error_message": ""
  }
}
```

Hinweise:

- `api_key_last4` ist absichtlich nur maskierte Metadaten
- der rohe API-Key wird vom Backend niemals zurückgegeben
- `message` variiert je nach Endpunkt und Ausgang

### Fehler-Antworten

Aktuelle Fehler-Antwort-Form:

```json
{
  "success": false,
  "message": "Gemini API key is required",
  "error_code": "missing_api_key",
  "field": "api_key"
}
```

Verbreitete aktuelle Fehlercodes:

- `missing_api_key`
- `invalid_api_key`
- `nvs_write_failed`
- `nvs_clear_failed`
- `not_configured`
- `task_alloc_failed`
- `task_start_failed`
- Provider-Fehlercodes aus Gemini-HTTP-Fehler-Payloads, wenn vorhanden

## UI-Integration

Followup nutzt Gemini-Bereitschaft aktuell auf zwei produktseitige Arten:

- die Statusleiste zeigt das Stern-Symbol, wenn WLAN verbunden und Gemini
  authentifiziert ist
- die Feedback-Ebene spielt einen eigenen Gemini-verbunden-Cue ab, wenn die
  Bereitschaft von false auf true wechselt

Das aktuelle Statusleisten-Symbol ist das bereits in `project_assets`
vorhandene Stern-Asset. Diese Firmware nutzt noch nicht die breiteren
Upstream-Gemini-spezifischen Seiten-/UI-Abläufe.

## Logging

Aktuelle Logs decken ab:

- Service-Initialisierung
- Auth-Start
- Auth-Erfolg
- Auth-Fehlschlag
- Unterdrückung veralteter Auth-Ergebnisse
- Fehlschläge bei der Backend-Routen-Registrierung
- NVS-Lese-/Schreib-/Lösch-Fehlschläge
- app-seitige Gemini-Ereignis-Snapshots in `app_shell`

Diese Logs sollen Inbetriebnahme und Backend-Integration erleichtern, bevor
der größere Gemini-Funktionsumfang portiert wird.

## Zurückgestellte Followup-Funktionen

Der Upstream-Followup-Gemini-Stack ist breiter als diese Implementierung.

Noch nicht portiert:

- Transkriptions-Jobs
- Zusammenfassungs-Erzeugung
- Token-Zählung
- Upload archivierter Audiodateien
- Provider-Worker-Queue, gemeinsam genutzt über mehrere Gemini-Job-Typen
- Frontend-Portal-UI für Gemini-Einstellungen

Wenn diese Funktionen portiert werden, sollte dieses Dokument erweitert statt
ersetzt werden, damit es für das tatsächliche Laufzeitverhalten dieser
Firmware in jeder Phase korrekt bleibt.
