# Local AI Service (Ersatz für Gemini)

Dieses Dokument beschreibt den geplanten Ersatz der Cloud-Gemini-
Integration durch ein vollständig lokales Backend, das auf dem
Haushalts-Server ("Kraken", ein Raspberry Pi 5) läuft. Es ersetzt
`docs/gemini-service.md` für die `folloup-waveshare`-Linie dieses Forks
-- das Gerät hat keinen Grund mehr, mit Google zu sprechen, sobald ein
lokales Modell denselben Job im selben LAN erledigt.

**Status: Code geschrieben und committet, nichts gebaut, nichts
deployt.** Die unten beschriebenen Firmware- und Frontend-Änderungen
wurden vollständig auf dem `local-ai-plan`-Branch vorgenommen. Dieser
Fork hatte zum Zeitpunkt dieser Zeilen keine ESP-IDF-Toolchain und kein
node/npm in der Umgebung installiert, in der das geschrieben wurde, also
**hat nichts davon kompiliert oder eine Typprüfung durchlaufen,
geschweige denn auf echter Hardware gelaufen**, und auf Kraken wurde
nichts verändert, um das zu bedienen. Siehe "Umsetzungsstand" am Ende
dieses Dokuments für den genauen Stand: was fertig ist, was geschrieben,
aber ungeprüft ist, und was noch fehlt.

## Warum Gemini überhaupt ersetzen

Besitzer-Entscheidung: Das Waveshare-ESP32-S3-Board (bestellt, noch
nicht in Händen) soll nicht von einem Cloud-Konto, einem gespeicherten
API-Key oder Googles Verfügbarkeit abhängen. Kraken betreibt bereits ein
lokales LLM (LM Studio, OpenAI-kompatible REST-API) und eine lokale
ASR-Pipeline (faster-whisper, über `imkopfhaben-brain`) für ein
unabhängiges Projekt. Beides wiederverwenden statt etwas Neues zu bauen.

## Was tatsächlich verifiziert wurde (2026-09-01, auf Kraken)

- LM Studios lokaler Server stellt ein OpenAI-kompatibles
  `POST /v1/chat/completions` bereit. Funktioniert nachweislich gegen
  `google/gemma-4-e2b` (aktuell geladen, 4,6B, `gemma4`-Architektur --
  das ist LM Studios Listing-Name für ein Modell der Gemma-3n-Klasse).
- **Audio-Eingabe wird vom Server nicht akzeptiert**, obwohl das Modell
  selbst nominell multimodal ist (eine `mmproj`-Datei ist geladen).
  Sowohl OpenAIs `input_audio`-Content-Part-Form als auch eine
  `audio_url`-Form probiert; beide wurden mit demselben Fehler abgelehnt:
  `'content' objects must have a 'type' field that is either 'text' or
  'image_url'`. **Keinen Transkriptions-Pfad über gemma/LM Studio
  bauen.** Transkription bleibt bei faster-whisper.
- **`gemma-4-e2b` ist ein Reasoning-Modell, und Reasoning ist teuer.**
  Ein trivialer Ein-Satz-Prompt verbrauchte standardmäßig 122
  Completion-Token -- 114 davon waren versteckter `reasoning_content`,
  nicht die eigentliche Antwort. Das Setzen von
  `"reasoning_effort": "none"` im Request unterdrückte Reasoning
  vollständig (0 Reasoning-Token, korrekte Antwort, deutlich schneller).
  **Dieses Feld ist in jedem Request Pflicht**, keine Optimierung --
  ohne es sprengen Prompts das lokale Context-Window mit
  Reasoning-Token, bevor überhaupt echte Ausgabe entsteht.
- **Es gibt keinen Token-Zähl-Endpoint.** `POST /v1/internal/tokenize`
  liefert `"Unexpected endpoint or method"`. `summary_service`s
  bestehender `gemini_service::CountTokens()`-Aufruf hat kein lokales
  Äquivalent und muss zugunsten der zeichenbasierten Schätzung entfallen,
  die der Code schon als Fallback hat (`EstimateTokenCount()`,
  `text.size() / 4`) -- einfach unbedingt nutzen, statt vorher den
  Provider zu fragen.
- Der Antworttext liegt unter `choices[0].message.content`, getrennt von
  `reasoning_content` -- kein `<think>`-Tag-Strippen nötig, anders als
  bei manchen anderen lokalen Reasoning-Modellen.
- LM Studios Server war nur auf `127.0.0.1` gebunden, aus dem LAN nicht
  erreichbar (und damit auch nicht vom ESP32-Board). Mit
  Besitzer-Bestätigung (2026-09-01) auf `0.0.0.0` umgebunden und
  Erreichbarkeit unter `http://192.168.178.215:1234/v1/models` von
  außerhalb von localhost bestätigt. **Noch nicht dauerhaft**: der
  Server wurde manuell neu gestartet (`lms server start --port 1234
  --bind 0.0.0.0`), es gibt keine systemd-Unit, und das Verhalten nach
  einem echten Reboot wurde nicht getestet. Bewusst nicht blind gebaut
  -- siehe Offene Anschlusspunkte.
- Krakens LAN-IP zum Zeitpunkt des Schreibens: `192.168.178.215`. Muss
  entweder als Kconfig-Default fest verdrahtet oder, besser, per mDNS
  (`kraken.local`) aufgelöst werden, falls der ESP32-mDNS-Stack das im
  selben Netz zuverlässig auflösen kann, in dem sich der WLAN-Dienst des
  Boards ohnehin schon anmeldet.
- `imkopfhaben-brain/ai_service.py` hat bereits ein sauberes,
  eigenständiges `transcribe_audio(audio_path: str) -> str` mit
  `faster_whisper.WhisperModel` (bereits geladen, bereits in Produktion
  für das imkopfhaben-Projekt bewährt). Es ist von imkopfhabens
  Notiz-/Kategorie-Pipeline entkoppelt -- gefahrlos von einer neuen,
  unabhängigen Route aus aufrufbar.

## Ziel-Architektur

```
ESP32-S3 board (folloup-waveshare)
  |
  |-- transcription  -->  POST http://kraken:PORT/api/transcribe-raw   (new, thin wrapper)
  |                         multipart or raw WAV body -> {"transcript": "..."}
  |                         backed by imkopfhaben-brain's ai_service.transcribe_audio()
  |
  |-- summaries/text -->  POST http://192.168.178.215:1234/v1/chat/completions
  |                         model=google/gemma-4-e2b, reasoning_effort=none
  |
  \-- readiness check -> GET  http://192.168.178.215:1234/v1/models
                            (replaces the old Gemini model-GET auth probe;
                            no credential needed)
```

Kein API-Key, kein in NVS gespeichertes Secret, kein TLS/CRT-Bundle
nötig für die LLM- oder Transkriptions-Aufrufe (beide sind reines HTTP
im LAN). Die `esp_crt_bundle_attach`-Nutzung in `gemini_service.cpp`
wird für diese Pfade zu totem Code.

## Firmware-Änderungen (geschrieben, ungeprüft -- siehe Umsetzungsstand)

Alle drei Aufrufstellen laufen über `components/gemini_service/`, der
Wirkungsradius ist also eingegrenzt -- `transcription_service`,
`summary_service`, `app_shell`, `status_bar_runtime`, `epaper_ui`
(Status-Stern) und `feedback_service` (Connect-Cue) konsumieren alle
`gemini_service::Snapshot` / `GenerateText` / `Transcribe` /
`CountTokens` und müssen nicht geändert werden, solange die öffentliche
API-Form erhalten bleibt und nur intern gegen das lokale Backend neu
implementiert wird. Empfehlung: die öffentliche Oberfläche der
Komponente stabil halten und nur das Innenleben austauschen, um eine
repo-weite Umbenennung ohne Möglichkeit, sie hier zu kompilieren und zu
prüfen, zu vermeiden.

1. **`kGeminiApiBaseUrl`** -> per Kconfig konfigurierbare lokale
   Basis-URL (`CONFIG_FOLLOWUP_LOCAL_AI_BASE_URL`, Default
   `http://192.168.178.215:1234/v1/`), kein `x-goog-api-key`-Header.
2. **`BeginAuthentication()` / Readiness-Probe** -> `GET {base}models`
   (`/v1/models`), 200 = bereit. Keine Key-Vorrangs-Logik nötig;
   "konfiguriert" kann einfach "hat eine Basis-URL" bedeuten, was immer
   wahr ist, sobald ein sinnvoller Default ausgeliefert wird.
3. **`GenerateText()`** -> `POST {base}chat/completions` mit
   `{"model":"google/gemma-4-e2b","messages":[{"role":"user","content":prompt}],"temperature":0,"reasoning_effort":"none"}`;
   `choices[0].message.content` statt
   `candidates[0].content.parts[].text` parsen.
4. **`CountTokens()`** -> den HTTP-Roundtrip löschen; Aufrufer
   (`summary_service::CountPromptTokens`) sollen unbedingt den
   bestehenden `EstimateTokenCount()`-Fallback nutzen.
5. **`Transcribe()`** -> den Gemini-Resumable-Upload-Tanz
   (`PerformUploadStart` / `PerformUploadFinalizePcmWav` /
   `generateContent` mit `fileData`) durch ein einzelnes `POST` desselben
   WAV-Bodys ersetzen (gebaut mit dem bestehenden
   `BuildWavHeaderPcm16Mono` + `RecordedClip::ForEachChunk`) an den neuen
   `/api/transcribe-raw`-Endpoint; `{"transcript": "..."}` parsen.
6. **Token-Budgets in `summary_service.cpp` müssen drastisch schrumpfen.**
   Sie waren auf Geminis riesiges Context-Window ausgelegt:
   `kSummaryInputTokenBudget=120000`, `kSummaryChunkTokenBudget=60000`,
   `kSummaryRollupTokenBudget=120000`. Der lokale Server läuft mit
   `--ctx-size 8192`. Diese müssen auf etwa
   `kSummaryInputTokenBudget≈3000`, `kSummaryChunkTokenBudget≈2500`,
   `kSummaryRollupTokenBudget≈3000` runter (lässt im 8192er-Window noch
   Raum für das Prompt-Gerüst und die Antwort) -- die genauen Zahlen
   sollten gegen echt gemessene Prompt-Größen justiert werden, sobald
   das Board existiert, nicht weiter geraten werden.
7. **Einstellungen-/API-Key-UI** (`epaper_ui/settings_page.cpp`, die
   Portal-Routen `/api/settings/gemini*` und das `webserver/src`-
   Frontend) gehen noch von einem gespeicherten Secret aus. Für diesen
   Durchlauf außerhalb des Umfangs -- unten geflaggt.

## Offene Anschlusspunkte (bewusst noch nicht erledigt)

- Keine systemd-Unit für LM Studios Server. Er muss einen Kraken-Reboot
  überstehen, damit das Board ein verlässliches Ziel hat; das wurde
  nicht blind gebaut und braucht einen echten Reboot-Test, der
  übersprungen wurde, um die laufende Maschine nicht mitten in der
  Sitzung zu stören.
- Es gibt noch keinen `/api/transcribe-raw`-Endpoint auf Kraken -- muss
  ergänzt werden (ein paar Zeilen, die `ai_service.transcribe_audio`
  wiederverwenden), und entweder als neue Route auf
  `imkopfhaben-brain` oder als winziger eigenständiger Dienst
  exponiert werden. Noch nicht gebaut, weil die genaue Hosting-Wahl
  sich danach richten sollte, wie der Besitzer Krakens wachsenden
  Haufen kleiner lokaler Dienste organisiert haben will, nicht hier
  einseitig entschieden werden sollte.
- Die C++-Änderungen oben sind ungeschrieben und ungeprüft -- die
  Build-Umgebung dieses Forks hatte keine ESP-IDF-Toolchain installiert,
  also hat hier nichts auch nur ein einziges Mal kompiliert.
- Einstellungsseiten-/Portal-/Frontend-Umbau für einen keyless Provider
  (Punkt 7 oben) ist nicht abgegrenzt.
- `kraken.local` mDNS vs. fest verdrahtete LAN-IP für die Basis-URL ist
  eine offene Wahl -- die IP ist einfacher und war das, was tatsächlich
  getestet wurde; mDNS ist robuster gegenüber einer sich ändernden
  Kraken-IP, fügt aber auf der ESP32-Seite einen Auflösungsschritt
  hinzu, der gegen den WLAN-/mDNS-Stack dieses Projekts noch nicht
  geprüft wurde.

## Umsetzungsstand (2026-09-02, dritter Durchlauf)

**In diesem Durchlauf kompiliert und verifiziert:** ESP-IDF v5.4 + npm
wurden eigens für diese Prüfung auf Kraken installiert. `idf.py build`
(Ziel esp32s3) läuft jetzt komplett durch -- `folloup_sticky.bin` wird
erzeugt, 65% App-Partitionsplatz frei. `npm run build` in `webserver/`
(tsc -b + vite build) läuft ebenfalls fehlerfrei durch, und das gebaute
Bundle wurde nach `components/wifi_service/portal/` kopiert (siehe
"Erledigt" unten -- das schließt die "bewusst unangetastet
gelassen"-Lücke aus dem vorigen Durchlauf). Zwei vorbestehende
Compile-Fehler, unabhängig von den lokalen-KI-Änderungen und schon im
`folloup-waveshare`-Branch vorhanden, bevor diese Arbeit überhaupt
begann, wurden dabei gefunden und gefixt -- siehe "In diesem Durchlauf
gefixt" unten. Das ist das erste Mal überhaupt, dass dieser Branch
erfolgreich kompiliert.

Weiterhin wahr: außer der LAN-Bindung von LM Studios Server und der
(aktuell nur syntaxgeprüften, nicht live getesteten)
`/api/transcribe-raw`-Ergänzung in `imkopfhaben-brain/main.py` wurde
auf Kraken nichts angefasst oder deployt. Keine systemd-Units
installiert. Es gibt noch keine echte Hardware, um das draufzuspielen
oder das Laufzeitverhalten zu prüfen (Netzwerk-Aufrufe, Audio, Display,
Energieverwaltung) -- ein sauberer Compile beweist, dass der Code
strukturell in Ordnung ist, nicht dass er auf dem Gerät funktioniert.

### Erledigt (geschrieben, entspricht dem Plan oben)

- `components/gemini_service/` umbenannt in `components/local_ai_service/`
  (Verzeichnis, Dateien, Namespace, CMakeLists `REQUIRES`). Die
  öffentliche API-Form blieb nah am Original, damit Aufrufer keinen
  strukturellen Umbau brauchten, nur umbenannte Aufrufe.
- Readiness-Probe, `GenerateText` und `Transcribe` wie spezifiziert
  gegen das lokale Backend neu implementiert (Basis-URL + `/models`,
  `/chat/completions` mit `reasoning_effort: "none"`, Single-POST-
  WAV-Upload an eine Transkriptions-URL).
- `CountTokens` / `TokenCountResult` komplett entfernt;
  `summary_service`s `CountPromptTokens` ruft jetzt direkt die
  bestehende Zeichen-Schätzung auf, kein Remote-Aufruf mehr.
- `summary_service`-Token-Budgets auf ein 8192-Token-lokales Context-
  Window geschrumpft (3000/2500/3000, runter von
  120000/60000/120000) -- das sind Startschätzungen, ausdrücklich
  nicht gegen echte Prompt-Größen justiert.
- Transkriptions-Readiness (sowohl in `transcription_service` als auch
  in `recording_session_service`) prüft jetzt "ist eine
  Transkriptions-URL konfiguriert", nicht mehr die Readiness des
  Chat-Modells -- fängt einen Bug im eigenen ersten Durchlauf ab, bei
  dem ich die alte Single-Provider-Sperre kopiert und zwei unabhängige
  lokale Dienste vermischt hatte.
- Jeder `gemini`/`Gemini`-Bezeichner, jede Log-Nachricht und jeder
  UI-String im Firmware-C++ und im `webserver/src`-TypeScript/HTML-
  Quellcode wurde gefunden (`rg -il gemini`, zweimal durchgekämmt) und
  umbenannt oder umformuliert, außer den unten als bewusst unangetastet
  gelisteten.
- Keine neuen Bild-Assets. Das bestehende Stern-Icon
  (`EmbeddedIconId::kStar`) wird unverändert für den Statusleisten-/
  Sperrbildschirm-"KI bereit"-Indikator weiterverwendet, wie zuvor.
- Oberste `README.md` und `webserver/README.md` aktualisiert, damit sie
  Gemini nicht mehr als aktuelles Backend beschreiben.
- `docs/gemini-service.md` oben als abgelöst markiert, sonst
  unverändert als historischer Nachweis belassen (Upstream und andere
  Branches dieses Forks können weiterhin Gemini-basiert sein).

### In diesem Durchlauf gefixt (2026-09-02): Readiness-/Validierungs-Bugs + vorbestehende Build-Fehler

- `local_ai_service.cpp` `Authenticate()`: Readiness meldete
  `success=true` bei jedem HTTP-2xx von `/v1/models`, auch wenn das
  konfigurierte Modell gar nicht in der zurückgegebenen Liste stand --
  ein falscher/nicht geladener Modellname zeigte sich als "verbunden",
  ohne Fehler. Jetzt `success = model_listed`, mit einer eigenen
  Statusmeldung ("... but configured model not loaded") statt des
  generischen "unreachable"-Texts.
- `local_ai_service.cpp` `ApplySettingsPatch()`: `base_url` wurde mit
  400 abgelehnt, wenn sie sich zu leer normalisierte; `transcribe_url`
  hatte keine entsprechende Prüfung und wäre still als leerer String
  gespeichert worden. Jetzt symmetrisch.
- `xpowers_axp2101_driver.cc:2504` (`log_d`-Format-String / `uint32_t`
  vs. `%x`-Mismatch) und `board_es8311_codec.cc` (`.bclk_div`-Feld, in
  dieser IDF-Version aus `i2s_std_clk_config_t` entfernt; der
  umgebende Kommentar sagte schon, dass es im Master-Modus wirkungslos
  ist, das Entfernen also verhaltenserhaltend ist) -- beide
  vorbestehend, beide unabhängig vom lokalen-KI-Umbau, beide blockierten
  *jeden* Build dieses Branches, unabhängig von den KI-Änderungen.
  Gefixt, weil ein sauberer Build der Zweck dieses Durchlaufs war;
  den exakten Diff siehe Git-Historie des Firmware-Repos.

### Bewusst unangetastet gelassen

- `components/project_assets/` (das `kGeminiApi`-Icon, aus
  `assets/icons/gemini_api.png`, und der zugehörige Eintrag in
  `assets/epaper_assets.json`) -- dieses generierte Asset war schon vor
  dieser Änderung von keiner Seite referenziert (bestätigt: keine
  Treffer für `EmbeddedIconId::kGeminiApi` außerhalb seiner eigenen
  Definition), es unangetastet zu lassen ändert also nichts am
  Verhalten. Es umzubenennen oder zu entfernen würde bedeuten, generierte
  Asset-Pipeline-Ausgabe anzufassen, was außerhalb des Umfangs lag
  ("keine neuen Assets").

### Design-Entscheidungen, die im Review nochmal angeschaut werden sollten

- `providerKeys.ts`: statt die Basis-URL ins alte maskierte-Secret-UI-
  Muster zu zwingen (`******last4`, versteckter Speichern-Button, sobald
  ein Key existiert), hab ich einen kleinen, eigenen Local-AI-Controller
  geschrieben, der die URL im Klartext zeigt und immer bearbeitbar
  bleibt -- eine URL ist kein Secret, und der ganze Sinn des alten
  Musters war, ein gespeichertes Secret nie wieder auf der Seite zu
  zeigen. Das hier als bewusste Abweichung von "exakt dasselbe Muster
  wiederverwenden" markiert, kein Versehen.
- Nur `base_url` ist in der Web-Einstellungen-UI exponiert;
  `transcribe_url` ist über das Backend-`PATCH /api/settings/local_ai`
  setzbar (nimmt beide Felder an), hat aber noch kein UI-Feld. Vorerst
  nur Kconfig-/NVS-Default.
- Mehrere Gemini-spezifische `RuntimeSnapshot`-/Fehlercode-Felder ohne
  lokales Äquivalent fallen gelassen, statt sie vorzutäuschen:
  `has_sdkconfig_api_key` (es gibt keine separate "gibt es einen
  Fallback"-Frage, wenn die Basis-URL immer einen Kconfig-Default hat),
  `api_key_last4` (nichts zu maskieren), und die
  `missing_api_key`/`invalid_api_key`-Fehlercodes (ersetzt durch
  `missing_fields`/`invalid_base_url`, da die Validierungsfrage jetzt
  eine URL betrifft, keinen Key). **Update (Review-Durchlauf,
  2026-09-02):** `supports_audio_understanding`/
  `supports_structured_output` wurden anfangs als dauerhaft-`false`-
  Felder mit einem erklärenden Kommentar beibehalten -- aber ein Review
  markierte sie als reinen toten Ballast (dauerhaft `false`, von
  keinem Portal-Frontend-Code je gelesen), also wurden sie seither
  komplett aus `RuntimeSnapshot` und dem Runtime-JSON entfernt, statt
  als inerte Platzhalter zu bleiben. Falls je echte Audio-
  Understanding-/Structured-Output-Unterstützung für ein anderes
  lokales Modell dazukommt, das Feld dann zurückbringen, angebunden an
  etwas, das tatsächlich `true` werden kann.

### Nicht erledigt / braucht eine Entscheidung, bevor es das sein kann

- `/api/transcribe-raw` existiert jetzt in `imkopfhaben-brain/main.py`
  auf Kraken (siehe `kraken-arche` PR #1s `transcribe_raw_patch.md`,
  angewendet), syntaxgeprüft (`python3 -m py_compile`), aber **nicht
  live getestet**. Der Dienst lief nicht, als das angewendet wurde
  (eigene Projekt-Konvention: nur manueller Start, kein Autostart), und
  ihn zum Testen zu starten wurde vom Auto-Mode-Classifier blockiert,
  plus eine inzwischen geklärte Sorge wegen versehentlichem Autostart --
  Details siehe PR #1 / Sitzungs-Log. Ein echter End-to-End-Test (WAV
  per POST schicken, prüfen ob das Transkript zurückkommt) ist weiterhin
  offen.
- Keine systemd-Unit für LM Studios Server; weiterhin ein manuell
  gestarteter Prozess, gebunden auf `0.0.0.0:1234`. Entwurf einer Unit
  liegt in `kraken-arche` PR #1, ungeprüft (kein Reboot-Test).
- Noch keine echte Hardware, um das draufzuspielen und laufen zu lassen
  (Board bestellt). Ein sauberer Compile ist keine Laufzeit-Garantie --
  Netzwerkverhalten, Audio, E-Paper-Refresh und Energieverwaltung sind
  alle unverifiziert, jenseits des Code-Lesens.
- **Automatisierter Review am 2026-09-02 erneut gelaufen, diesmal
  abgeschlossen** (teilweise -- 4 von 8 Finder-Agent-Angles kamen nicht
  rechtzeitig zurück; der Koordinator hat aus der eigenen manuellen
  Durchsicht plus den 4 zurückgekommenen Angles fertiggestellt, statt
  die fehlenden zu erfinden). Funde:
  - **Gefixt, gegen die Vor-Diff-Fassung von `PerformGet`/
    `PerformJsonPost` zum Vergleich verifiziert:** `PostWavClip()`
    setzte sowohl einen `event_handler` (der jeden
    `HTTP_EVENT_ON_DATA`-Chunk an `response.body` anhängt) *als auch*
    drainte den Body nach `fetch_headers` manuell erneut über
    `esp_http_client_read()` -- ESP-IDF feuert dieses Event auch bei
    manuellen `read()`-Aufrufen, nicht nur bei `_perform()`, also kam
    jede echte Transkriptions-Antwort doppelt angehängt zurück
    (`{"transcript":"x"}{"transcript":"x"}`), was das JSON-Parsen bei
    jeder echten Aufnahme gebrochen hätte. Gefixt, indem der Event-
    Handler aus genau dieser einen Funktion entfernt wurde (sie ist die
    einzige der drei HTTP-Aufrufstellen, die manuell liest statt
    `_perform()` zu nutzen).
  - **Bestätigt real, nicht gefixt -- braucht eine Entscheidung:**
    Transkriptions-Readiness (`recording_session_service.cpp`) prüft
    jetzt nur noch "ist eine Transkriptions-URL konfiguriert" (ein
    statischer Konfigurationsstring), keinen Live-Erreichbarkeits-
    Check -- anders als `base_url`/das Chat-Modell, das einen
    `Authenticate()`-Roundtrip hat. Ein offline Gerät oder ein
    ausgefallener Transkriptions-Server blockiert jetzt für das volle
    `kTranscribeTimeoutMs` (30s) pro Versuch, statt schnell zu
    scheitern. Das richtig zu fixen bedeutet, ein Health-Check-
    Subsystem für den Transkriptions-Endpoint symmetrisch zu
    `Authenticate()` zu ergänzen -- als echte Design-Entscheidung
    eingeordnet, kein Ein-Zeilen-Patch.
  - Erneut bestätigt, nicht neu: `transcribe_url` hat weiterhin kein
    UI-Feld (schon oben unter "Design-Entscheidungen" gelistet).
  - Niedrigerpriorisierte Duplikations-/Effizienz-Funde (von mir nicht
    unabhängig nachverifiziert, unverändert aus den abgeschlossenen
    Finder-Angles übernommen): wiederholtes HTTP-Client-Boilerplate
    über `PerformGet`/`PerformJsonPost`/`PostWavClip` hinweg;
    `providerKeys.ts` implementiert den Busy-Flag-/Fehler-Wrapper
    erneut, der an anderer Stelle in derselben Datei schon
    generalisiert ist; `BuildChatCompletionRequestBody` implementiert
    den bestehenden `JsonString()`-Helfer erneut, statt ihn
    aufzurufen; `summary_service`s Eingabe-Trimm-Schleife baut bei
    jedem entfernten Eintrag den kompletten restlichen Prompt neu auf
    und scannt ihn erneut, was jetzt unter dem ~40x kleineren lokalen
    Token-Budget deutlich häufiger passiert; `GenerateSummary()` holt
    `base_url`/`model_name` erneut aus einem Snapshot, den es schon
    besitzt.
- **Gebaut (2026-09-02): Prototyp-Retry für fehlgeschlagene
  Transkriptions-Uploads, damit das Gerät mitgenommen werden kann und
  die Notizen zugestellt werden, sobald es wieder in Reichweite von
  Kraken/LM Studio ist.** Gleiche Absicht wie `imkopfhaben`s
  `notiz_warteschlange/`, brauchte aber fast keine neue Verkabelung --
  jede Aufnahme wird schon auf SD gesichert, bevor Transkription
  überhaupt versucht wird (`recording_archive_service::SaveClip`), jeder
  Eintrag trägt schon ein `has_transcript`-Flag, und ein kompletter
  Lade-Clip -> transkribieren -> Transkript-speichern-Retry-Pfad
  existierte schon für die manuellen "Retry"-Buttons auf den
  Details-/Vibe-Check-Seiten (`BeginArchivedTranscription`). Der einzige
  neue Code steckt in `recording_session_service.cpp`:
  `TryRetryOldestUnsentRecording()` geht `ListRecordings()` durch,
  findet den ersten Eintrag mit `has_transcript == false`, und schickt
  ihn erneut durch genau diese bestehende Pipeline (ein Versuch nach dem
  anderen -- `BeginTranscription` läuft ohnehin immer nur eine Anfrage
  gleichzeitig, und einen einzelnen Eintrag erneut zu versuchen hält
  einen ausgefallenen Server davon ab, in einer engen Schleife
  bombardiert zu werden). Zwei Auslöser, beide ausdrücklich flach/ohne
  Drosselung -- keine batteriebewusste Rückstellung, passend zum
  Besitzer-Umfang "funktionaler Prototyp, keine Produktion": ein
  simpler 10-Minuten-`esp_timer` (`kTranscriptionRetryIntervalUs`), und
  eine sofortige Folgeprüfung direkt nach jeder erfolgreich
  gespeicherten Transkription (arbeitet die Warteschlange zügig ab,
  sobald die Verbindung bestätigt steht, statt das volle Intervall
  abzuwarten). Kompiliert sauber, inklusive eines frischen `idf.py
  fullclean && idf.py build` nachdem `esp_timer` zu den `REQUIRES` der
  Komponenten-`CMakeLists.txt` ergänzt wurde (baute auch ohne das schon
  im früheren inkrementellen Build, über einen transitiven Include-Pfad
  -- auf eine explizite Abhängigkeit umgestellt, statt sich darauf zu
  verlassen). **Nicht live getestet** (noch kein Board) -- nur
  code-seitig, gleicher Vorbehalt wie alles andere in diesem Dokument,
  bis echte Hardware ankommt.
