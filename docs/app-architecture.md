# Followup App-Architektur

Dieses Projekt ist eine ESP-IDF-C++17-Firmware-Anwendung für das
[Waveshare ESP32-S3-ePaper-3.97](https://docs.waveshare.com/ESP32-S3-ePaper-3.97).
Die Board-Details stehen in `docs/waveshare-epaper-hardware-spec.md`.

Die Codebasis begann als Portierung für den Seeed reTerminal Sticky, und
Teile dieses Dokuments beschreiben diese Herkunft noch dort, wo die
Design-Begründung übernommen wurde. Wo sich die beiden Boards
unterscheiden, beschreibt dieses Dokument die Waveshare-Hardware, die
aktuell das einzige Ziel ist, für das die Firmware gebaut wird. Die
wichtigsten Unterschiede:

| Bereich | reTerminal Sticky | Waveshare ESP32-S3-ePaper-3.97 |
| --- | --- | --- |
| Eingabe | GT911 kapazitives Touch + Tasten | Nur Tasten — kein Touch-Controller |
| Mikrofon | PDM-Mikrofon (`pdm_mic`, `microphone_service`) | ES8311-Codec über I2S (`audio_hal`) |
| Lautsprecher | keiner | ES8311 + NS4150B-Verstärker |
| Strom | BQ27220-Fuel-Gauge + diskreter Latch | AXP2101-PMIC (Rails, Ladegerät, Power-Taste) |
| RTC | PCF85063 | PCF85063 |
| IMU | — | QMI8658 6-Achsen |
| Display-Bus | SPI2 gemeinsam mit SD | Eigener SPI3, kein geteilter Bus |

## Aktueller Umfang

Das Repository ist eine mehrseitige ESP-IDF-Produktanwendung
(Dashboard-Startseite, Onboarding und eine Reihe von Feature-Seiten plus
Overlays), aufgebaut auf:

- ESP32-S3-Zielkonfiguration.
- 16-MB-Flash-Konfiguration.
- 8-MB-PSRAM-Konfiguration.
- OTA-fähiges Partitions-Layout mit aktiviertem Rollback.
- Ein minimales C++ `app_main()`.
- Eine `axp2101`-Komponente: der PMIC-Treiber, der die Rails, das
  Ladegerät, die Akku-Telemetrie und den Power-Taste-Interrupt-Stream
  besitzt.
- Ein portierter PCF85063-RTC-Treiber.
- Eine `qmi8658`-Komponente für den 6-Achsen-IMU, der für das
  Bewegungs-Aufwachen genutzt wird.
- Eine `board`-Komponente (`waveshare_board`), die die Waveshare-
  spezifische Pin-Zuordnung, das PMIC-Rail-Hochfahren, den geteilten
  Sensor-I2C-Bus und die Audio-Codec-Instanz besitzt.
- Eine `power_service`-Komponente, die die Strom-Hardware initialisiert
  und einen diagnostischen Strom-/Akku-/RTC-Snapshot loggt.
- Eine `button_service`-Komponente, die App-seitige Tasten-Ereignisse
  über Espressifs verwaltete Button-Komponente loggt.
- Eine `audio_hal`-Komponente, die den ES8311-Codec für vollduplex
  16-kHz-Aufnahme und -Wiedergabe kapselt, inklusive Aktivierung des
  NS4150B-Verstärkers.
- Eine `system_sound_service`-Komponente, die den dekodierten
  Sound-Cue-Katalog besitzt und Cues an den Codec streamt.
- Eine `feedback_service`-Komponente, die die App-seitige
  Interaktions-Feedback-Policy besitzt und App-Ereignisse auf Sound-Cues
  abbildet.
- Eine `playback_service`-Komponente, die einen Clip an den Codec
  streamt, entweder aus einer WAV-Datei auf der SD-Karte oder direkt aus
  den PSRAM-Chunks, die `recording_service` hält.
- Eine `design_tokens`-Komponente, die gemeinsame Produkt-UI-Konstanten
  wie Abstände, Farben, Typografie-Rollen und Komponentengrößen besitzt.
- Eine `epaper_ui`-Komponente, die die wiederverwendbaren
  E-Paper-Präsentations-Primitiven besitzt (Statusleiste, globale
  Fußzeile, Sperrbildschirm, Karten-Modal, Auswahl-Modal, Toast,
  Tastatur, Karussell, Scroll-Container, Timeline-Liste, Sticky Note und
  die vielen Listen-/Menü-/Eingabe-Widgets) plus die vollständigen
  Seiten-Renderer (Dashboard, Onboarding, Vibe Check, Zusammenfassen,
  Notizen, Todos, Follow-up, Details, Einstellungen, WLAN, Zeit).
- Eine portierte `sd_card`-Komponente für SDMMC/FATFS-MicroSD-Zugriff.
- Eine `storage_service`-Komponente, die die App-seitige MicroSD-Mount-,
  Format- und Debug-Status-Policy besitzt.
- Eine `wifi_service`-Komponente, die den ESP-IDF-WLAN-
  Station-/AP-Lebenszyklus, gespeicherte Zugangsdaten, Scan-Status und
  die Backend-HTTP-Routen für Einrichtung/Status besitzt.
- Eine `timezone_service`-Komponente, die Zeitzonen-Einstellungen,
  SNTP-Sync, System-Zeit-Updates, PCF85063-RTC-Rückschreiben und
  Backend-HTTP-Routen für Zeit-Einstellungen/Runtime-Status besitzt.
- Eine `gemini_service`-Komponente, die die Vorrang-Regeln für
  Gemini-API-Key-Einstellungen, Backend-HTTP-Routen und den
  Gemini-Authentifizierungs-Bereitschaftsstatus besitzt.
- Eine `recording_service`-Komponente, die den Sprach-Eingabe-
  Aufnahmestatus, Pre-Roll-Pufferung, PSRAM-gestützte Clips,
  Eingangspegel-Tracking und WAV-Export auf die MicroSD besitzt.
- Eine `recording_session_service`-Komponente, die den
  Drücken/Halten-Aufnahme-Ablauf, die Start-/Stop-Sound-Cues, die
  Review-Wiedergabe der Aufnahme, die Tag-Auswahl und die
  Speichern-/Transkriptions-Orchestrierung besitzt.
- Eine `recording_archive_service`-Komponente, die den SD-Aufnahme-Index
  (Auflistung, Metadaten, Follow-up-Flags) besitzt, der von den
  Notizen-/Todos-/Follow-up-Seiten und dem Sticky-Note-Overlay angezeigt
  wird.
- Eine `transcription_service`- und eine `summary_service`-Komponente,
  die die Gemini-gestützten Transkriptions- bzw. Zusammenfassungs-Abläufe
  besitzen (diese leben in eigenen Komponenten, nicht innerhalb von
  `gemini_service`).
- Ein portierter Mono-SSD1677-E-Paper-Panel-Treiber. Auf diesem Board
  besitzt das Panel einen eigenen SPI3-Bus, es gibt also keine
  geteilte-Bus-Serialisierung zu tun — das `shared_bus_service` des
  Sticky hat hier kein Gegenstück.
- Eine `display_service`-Komponente, die das App-seitige E-Paper-
  Hochfahren, den Leer-Bildschirm-Refresh, den Display-Sleep und die
  Light-Sleep-Wiederherstellung besitzt.
- Vorbereitete E-Paper-Asset-Generierungs-Skripte und Quell-PNG/TTF-
  Assets für die App-UI.

- Ein portierter QMI8658-6-Achsen-Trägheitssensor-Treiber.
- Eine `imu_service`-Komponente, die das App-seitige IMU-Hochfahren und
  Sample-Logging besitzt, genutzt vom bewegungsbasierten Aufwachen.
- Eine `device_sleep_service`-Komponente, die den Auto-Sleep-Policy-
  Status, Inaktivitäts-Timing, App-seitige Blocker-Prüfungen und
  gestufte Sleep-Ereignisse besitzt.
- Eine `task_config`-Komponente, die die von der App erzeugte
  FreeRTOS-Task-Prioritäts- und Core-Affinitäts-Zuordnung besitzt.

Das Board trägt einen SHTC3-Temperatur-/Feuchtigkeitssensor am geteilten
Sensor-I2C-Bus. Er wird aktuell von keiner Komponente angesteuert.

Dieses Board hat keinen Touch-Controller — die Eingabe erfolgt
ausschließlich über Tasten. Manch ein Widget-Code trägt noch
`kTouch*`-Hit-Slop-Konstanten und einen `kTouchContact`-Feedback-Cue aus
der Sticky-Portierung; die sind Überbleibsel, nichts dispatcht
Touch-Ereignisse.

## Projekt-Layout

```text
CMakeLists.txt
assets/
  epaper_assets.json
  icons/
  logos/
fonts/
main/
  CMakeLists.txt
  main.cpp
  app_shell.h
  app_shell.cpp
  button_input_runtime.h
  button_input_runtime.cpp
  device_sleep_runtime.h
  device_sleep_runtime.cpp
  footer_runtime.h
  footer_runtime.cpp
  input_callback_dispatcher.h
  input_callback_dispatcher.cpp
  input_focus_runtime.h
  input_focus_runtime.cpp
  input_runtime_setup.h
  input_runtime_setup.cpp
  lock_screen_runtime.h
  lock_screen_runtime.cpp
  status_bar_runtime.h
  status_bar_runtime.cpp
  page_action_result.h
  overlay_runtime.h
  overlay_runtime.cpp
  page_input_runtime.h
  page_input_runtime.cpp
  shared_page_interactions.h
  # Pro-Seite-Runtime-Familien. Jede Feature-Seite hat ein {runtime, coordinator,
  # interactions}-Trio (settings/wifi/time stammen von vor der Coordinator-Aufteilung
  # und halten ihren Status weiterhin in der runtime):
  #   dashboard_page_*  onboarding_page_*  vibe_check_page_*  summarize_page_*
  #   notes_page_*  todos_page_*  follow_up_page_*  details_page_*
  #   settings_page_{runtime,coordinator,interactions}  wifi_page_*  time_page_*
  settings_page_interactions.h
  settings_page_interactions.cpp
  wifi_page_interactions.h
  wifi_page_interactions.cpp
  time_page_interactions.h
  time_page_interactions.cpp
  page_interaction_runtime.h
  page_interaction_runtime.cpp
  timeline_format.h                # gemeinsame Timeline-Datum/Zeit-Formatierer ("Heute"-Logik)
  timeline_format.cpp
  ui_refresh_runtime.h
  ui_refresh_runtime.cpp
  app_interaction_result.h
  app_interaction_target.h
components/
  audio_hal/
  axp2101/
  board/
  button_service/
  design_tokens/
  device_sleep_service/
  display_service/
  epaper_panel/
  epaper_ui/
  feedback_service/
  gemini_service/
  i2c_device/
  imu_service/
  page_navigation/
  pcf85063/
  playback_service/
  power_service/
  project_assets/
  qmi8658/
  recording_archive_service/
  recording_service/
  recording_session_service/
  sd_card/
  storage_service/
  summary_service/
  system_sound_service/
  task_config/
  timezone_service/
  transcription_service/
  wifi_service/
partitions.csv
sdkconfig
sdkconfig.defaults
docs/
  app-architecture.md
  asset-generation.md
  auto-sleep.md
  gemini-service.md
  waveshare-epaper-hardware-spec.md
scripts/
  generate_epaper_assets_common.py
  generate_epaper_footer_icons.py
  generate_epaper_fonts.py
  generate_epaper_icons.py
  generate_epaper_logos.py
  generate_epaper_project_assets.py
```

## Komponenten-Grenzen

### `components/project_assets`

Diese Komponente besitzt eingebettete Assets, die ins Firmware-Image
kompiliert werden. Die Quell-PNG/TTF-Dateien und Generator-Skripte liegen
außerhalb der Komponente; diese Komponente stellt nur die generierten
C++-Daten und eine kleine App-seitige Lookup-API bereit.

Aktueller Umfang:

- gepackte Monochrom-Bild-Metadaten über `asset_types.h`
- Manifest-gesteuerte Asset-Generierung über `assets/epaper_assets.json`
- generierte E-Paper-Logo-Assets für die ALXV-Labs- und Folloup-Logos
- generierte E-Paper-Icon-Assets für alle PNG-Dateien, die aktuell in
  `assets/icons/` liegen
- leeres generiertes Footer-Icon-Gerüst, bereit für künftige
  Manifest-Einträge
- `project_assets::GetLogo(...)`, `GetIcon(...)` und `GetFooterIcon(...)`
  als Lookup-Helfer über generierte Enum-IDs

Die generierten Dateien reproduzierbar aus `assets/` und `scripts/`
halten. Generierte Asset-Quelldateien nicht von Hand bearbeiten. Siehe
`docs/asset-generation.md`.

### `components/design_tokens`

Diese reine Header-Komponente besitzt gemeinsame Produkt-UI-Konstanten.
Sie heißt absichtlich `design_tokens`, nicht E-Paper-Design-Tokens, weil
die Werte die Folloup-UI-Sprache beschreiben, nicht den
SSD1677-Display-Treiber.

Aktueller Umfang:

- Abstandsskala
- eine kanonische vierstufige E-Paper-Graustufen-Rampe (`gray1` bis
  `gray4`) plus semantische Graustufen-Farbrollen
- Typografie-Rollen, -Größen und -Gewichte
- gemeinsame Komponenten-Größenkonstanten, genutzt von der portierten
  E-Paper-UI

Diese Komponente als erste Abhängigkeit nutzen, wenn kleine Teile aus dem
alten `epaper_lib` portiert werden. UI-Tokens unabhängig von
Display-Hardware und Framebuffer-Mechanik halten.

### `components/epaper_ui`

Diese Komponente besitzt wiederverwendbare E-Paper-UI-Primitiven, die in
den Portrait-Framebuffer der App rendern. Sie hängt von `design_tokens`
für visuelle Konstanten und von `project_assets` für eingebettete
Icons/Logos ab, hängt aber nicht von App-Diensten oder
Start-/Runtime-Policy ab.

Aktueller Umfang:

- generierte Inter-Bitmap-Fonts, genutzt von den E-Paper-UI-
  Typografie-Rollen
- ein kleiner rollenbewusster Bitmap-Font-Renderer
- der App-Statusleisten-Zustands-Vertrag und -Renderer
- ein wiederverwendbarer Sperrbildschirm-Renderer plus sein eigener
  Zustands-Vertrag

App-eigene Runtime-Helfer in `main/` dürfen Dienst-Status in diese
UI-Verträge komponieren, aber die Zeichen-Primitiven selbst sollen
wiederverwendbar und dienst-unabhängig bleiben.

### UI-Schichtung

Der E-Paper-UI-Stack ist jetzt bewusst in drei Schichten aufgeteilt:

- `design_tokens` besitzt produktweite Abstände, Graustufen, Typografie
  und Komponenten-Maße.
- `epaper_ui` besitzt wiederverwendbare Präsentations-Primitiven wie
  Bitmap-Fonts, rollenbewusstes Text-Rendering, den Statusleisten-
  Renderer, den Sperrbildschirm-Renderer und künftige, aus `followup`
  portierte View-Widgets.
- App-eigene Runtime-Helfer in `main/` komponieren Dienst-Status in
  UI-Verträge. Heute gehört dazu `status_bar_runtime`, das
  `power_service`-, `wifi_service`-, `timezone_service`- und
  Sleep-/Shutdown-Status in ein neutrales `epaper_ui::StatusBarState`
  übersetzt, `footer_runtime`, das das Footer-Layout plus die
  geteilte-Fokus-Projektion in `epaper_ui::GlobalFooterState`
  projiziert, `overlay_runtime`, das die persistenten
  Modal-/Toast-UI-Verträge besitzt, und `lock_screen_runtime`, das Zeit
  plus Status-Indikatoren in `epaper_ui::LockScreenState` und die
  Sperrbildschirm-Sichtbarkeit komponiert.

`display_service` bleibt Besitzer des physischen Panels, des
Framebuffers, der Refresh-Modus-Entscheidungen und der
Sleep-/Wake-Übergänge. Es darf `epaper_ui`-Renderer konsumieren, sollte
aber nicht zur Heimat für Produkt-Status-Komposition oder eine
Grab-Bag-Sammlung wiederverwendbarer Widgets werden.

### Bildschirme und Overlays

Bildschirme sind sich gegenseitig ausschließende Vollbild-Underlays,
ausgewählt per `ScreenId`. Overlays legen sich über den aktiven
Bildschirm und fangen (außer dem Toast) währenddessen die Eingabe ab.

Bildschirme (`ScreenId`):

- `kHome` — das Dashboard: ein fokussierbares Menü, das die
  Feature-Seiten öffnet.
- `kOnboarding` — ein Erstboot-Karussell (Schließen / Zurück / Weiter).
  Wird einmal gezeigt, gesteuert über NVS `app_state`/`onboarded`;
  erneut startbar über Einstellungen → "Manuell".
- `kVibeCheck`, `kSummarize` — KI-Idee-/Zusammenfassungs-Karten.
- `kNotes`, `kTodos`, `kFollowUp` — Aufnahme-Timelines (zweistufig:
  Datums-Gruppen-Chips → eine betretene, scrollbare Elementliste),
  aufgebaut auf der `timeline_list`-Primitive und `timeline_format` (die
  "Heute"/absolute-Datum-Beschriftungen).
- `kDetails` — die Details einer einzelnen Aufnahme mit einem betretenen
  Transkript-Scroll-Container.
- `kSettings` (WLAN-/AP-Umschalter, SD formatieren, "Manuell"-
  Onboarding), `kWifi`, `kTime`, `kLockScreen`.

Overlays (`overlay_runtime` + gezeichnet in
`display_service::DrawCurrentOverlays`, Z-Reihenfolge Tastatur → Toast →
Auswahl-Modal → Karten-Modal → Sticky Note):

- **Karten-Modal** — Shutdown-/Speicher-Bestätigungen (löst das alte
  "Shutdown-Modal" ab).
- **Auswahl-Modal** — Einfachauswahl-Picker (z. B. Tag / Zeitzone).
- **Tastatur** — Texteingabe am Bildschirm.
- **Toast** — flüchtiger Status, optional schließbar.
- **Sticky Note** — ein Vollseiten-Overlay, geöffnet über den
  Fußzeilen-Sticky-Button, das durch die Follow-up-Notizen blättert
  (Zurück/Weiter mit Umlauf, Schließen), mit einem Details-artigen
  Scroll-Container pro Transkript.

### Unterdrückung des Overlay-Refresh

Alle Seiten-, Statusleisten-, Fußzeilen- und Overlay-Neuzeichnungen
laufen über `ui_refresh_runtime`, einen keyed-latest-wins-Worker. Jeder
Aufrufer plant einen *Apply-Callback* (der frischen Status in
`display_service` schiebt) plus eine Refresh-Anfrage, gekeyt per
`SurfaceKey` (`kOverlay`, `kLockScreen`, `kStatusBar`, `kFooter`, und je
ein Key pro Seite: `kSettingsPage`, `kWifiPage`, `kTimePage`,
`kDashboardPage`, `kVibeCheckPage`, `kSummarizePage`, `kNotesPage`,
`kTodosPage`, `kFollowUpPage`, `kDetailsPage`, `kOnboardingPage`). Der
Worker fasst anstehende Arbeit pro Surface zusammen und löst pro Drain
höchstens einen Bildschirm-(Underlay-)Refresh und einen Overlay-Refresh
aus.

**Die Overlay-Regel:** solange ein Overlay den Bildschirm besitzt — also
solange `overlay_runtime::IsInputCaptured()` wahr ist (Tastatur,
Auswahl-Modal, Karten-Modal, das Sticky-Note-Overlay, eine laufende
Shutdown-Anfrage oder ein schließbarer Toast) —, *unterdrückt der Worker
Underlay-Refreshes* (Seite/Status/Fußzeile). Die Apply-Callbacks laufen
trotzdem weiter, sodass der gespeicherte Status aktuell bleibt; nur der
Panel-Neuaufbau *unter* dem offenen Overlay wird ausgelassen.
Overlay-Refreshes (das Overlay zeichnet sich selbst neu) laufen immer
durch. Schließt das Overlay, greift beim nächsten Underlay-Refresh wieder
der neueste Status und der Bildschirm zeichnet sich neu (der
Overlay-Schließen-Pfad fordert diesen Refresh bereits an).

Das ist eine einzige globale Policy, an einer Stelle durchgesetzt
(`UiRefreshTask`), gilt also einheitlich für jeden Bildschirm statt pro
Event-Handler neu implementiert zu werden. Sie existiert, weil das
Neuaufbauen einer Seite unter einer offenen Tastatur oder einem Modal —
zum Beispiel bei jedem Uhr-Tick beim Bearbeiten der Zeit-Seite —
Overlays träge wirken ließ und, bei schneller Overlay-Navigation, den
Display-Task lange genug beschäftigt halten konnte, um den Idle-Task
auszuhungern und den Task-Watchdog auszulösen. Hintergrund-Ereignisse
(Uhr-Ticks, WLAN-/Scan-Updates, Akku-Änderungen) halten ihre Verträge
deshalb synchron, ohne einen sichtbaren Underlay-Neuaufbau zu erzwingen,
während der Nutzer in einem Overlay beschäftigt ist. Auto-Dismiss-Toasts
fangen keine Eingabe ab, sie unterdrücken also nie Underlay-Refreshes.

Invariante: jeder `SurfaceKey` muss auf einen eigenen Slot in
`ui_refresh_runtime` abbilden (`SurfaceIndex` plus `kSurfaceCount`). Ein
fehlender `SurfaceIndex`-Fall aliast diese Surface still auf Slot `0`
(`kOverlay`) und lässt Seiten-Refreshes den anstehenden
Overlay-Refresh von Tastatur/Modal überschreiben — beim Hinzufügen eines
Bildschirms synchron halten.

### `main`

`main/` besitzt die Produkt-Komposition für diese Firmware. Es ist keine
wiederverwendbare Komponente. Auf Start-Reihenfolge und App-Ebene-
Orchestrierung fokussiert halten.

`main/main.cpp` ist bewusst winzig: es ist nur der ESP-IDF-
`app_main()`-Einstiegspunkt und delegiert an `app_shell::Run()`.

`main/app_shell.cpp` ist nur eine Orchestrierungs-Schicht. Es darf die
Start-Reihenfolge entscheiden, App-Ebene-Policies verbinden und
entscheiden, ob ein Ausfall eines optionalen Dienstes fatal ist, sollte
aber keine Hardware-Treiber-Logik, Protokoll-Logik, Tasten-Entprellung,
Akku-Mathematik, Display-Zeichnen, Netzwerk-Abläufe oder lang laufende
Feature-Schleifen enthalten. Diese Verhaltensweisen gehören in
Dienste/Komponenten, aufgerufen aus der App-Shell.

`main/status_bar_runtime.cpp` ist ein Beispiel für das beabsichtigte
App-Runtime-Helfer-Muster. Es ist keine wiederverwendbare Komponente und
besitzt weder Hardware noch Rendering. Seine Aufgabe ist es,
Produkt-Status in UI-seitige Daten-Verträge zu komponieren, die
`display_service` über `epaper_ui` rendern kann.

Die aktuellen App-Runtime-Helfer unter `main/` sind:

- `status_bar_runtime`: komponiert WLAN-, Gemini-, Akku-, Sleep- und
  Shutdown-Status in `epaper_ui::StatusBarState`
- `footer_runtime`: projiziert Fußzeilen-Layout und geteilten
  Seiten-Fokus in `epaper_ui::GlobalFooterState`
  (Einstellungen/WLAN/Zeit/Ordner/Sticky/Home + Mikrofon)
- `overlay_runtime`: besitzt persistenten Overlay-Status (Karten-Modal,
  Auswahl-Modal, Tastatur, Toast und das Vollseiten-Sticky-Note-Overlay),
  Hit-Testing und Overlay-Präsentations-Hooks
- eine Runtime-Familie pro Feature-Seite — `{dashboard, onboarding,
  vibe_check, summarize, notes, todos, follow_up,
  details}_page_{runtime, coordinator, interactions}`, plus
  `settings/wifi/time` (runtime + interactions) — komponiert
  Seiten-Status und übersetzt Fokus in neutrale Seiten-Ergebnisse plus
  Folge-Intents
- `timeline_format`: gemeinsame Datum/Zeit-Formatierer für die
  Notizen-/Todos-/Follow-up-Timelines und das Sticky-Note-Overlay (die
  "Heute"-vs-absolutes-Datum-Logik)
- `input_runtime_setup`: besitzt die App-seitige Tasten-/Touch-Bindung
  plus das gemeinsame Eingaben-aktiviert-Gate, bevor Ereignisse ins
  App-Routing gelangen
- `input_focus_runtime`: besitzt Overlay-zuerst-Tasten-Routing für
  umlaufende Fokus-Bewegung plus App-weite Touch-Kontakt-Priorität
- `page_input_runtime`: besitzt das aktive Seiten-Eingabe-Routing für
  aktuell seiteneigene Bildschirme, inklusive Fokus-Bewegung, seitenlokale
  Tasten-Aktivierung, Fußzeilen-Projektions-Hooks, Touch-Provider-
  Registrierung und das Anwenden neutraler Seiten-Interaktions-Ergebnisse
  auf App-seitiges Verhalten
- `settings_page_interactions` / `wifi_page_interactions` /
  `time_page_interactions`: besitzen seitenlokale Fokus- und
  Aktivierungs-Semantik für die aktuell seiteneigenen Bildschirme, sodass
  die gemeinsame Seiten-Eingabe-Schicht Intent/Ergebnisse anwendet statt
  Seiten-Verhalten offen in jeder Runtime zu codieren
- `page_interaction_runtime`: besitzt den Registrierungs-Vertrag, über
  den künftige Seiten-Runtimes/Coordinators ihre Seiten-Ziele in den
  gemeinsamen Touch-Interaktionspfad einklinken
- `lock_screen_runtime`: besitzt Sperrbildschirm-Sichtbarkeit und
  Uhr-Status-Komposition
- `ui_refresh_runtime`: besitzt den keyed-latest-wins-UI-Präsentations-
  Worker und setzt die globale Overlay-Refresh-Regel durch (siehe
  "Unterdrückung des Overlay-Refresh")

Die aktuelle frühe Start-Sequenz ist:

- Erkennt, ob das laufende Image `ESP_OTA_IMG_PENDING_VERIFY` ist.
- Markiert das Image als gültig mit
  `esp_ota_mark_app_valid_cancel_rollback()`.
- Fährt die AXP2101-Rails hoch, noch vor der OTA-Validierung.
- Initialisiert `power_service`.
- Loggt einen diagnostischen Strom-/Akku-Snapshot.
- Initialisiert `feedback_service` und fordert das Start-Feedback an.
- Initialisiert `storage_service` und loggt einen diagnostischen
  MicroSD-Snapshot. Auf diesem Board muss, wenn eine Karte steckt, der
  Speicher vor dem geteilten-Bus-Display-Pfad initialisiert werden,
  damit die Karte zuerst in den SPI-Modus geht und gemountet bleibt.
- Initialisiert `display_service` und löscht das E-Paper-Panel auf einen
  leeren Bildschirm.
- Initialisiert `ui_refresh_runtime`, das den gemeinsamen
  latest-wins-UI-Präsentations-Worker besitzt.
- Initialisiert `overlay_runtime`, das globalen Modal-/Toast-Overlay-
  Status, Shutdown-Bestätigungs-Fokus und Overlay-Präsentations-Hooks
  besitzt.
- Initialisiert `imu_service` und loggt drei direkte IMU-Samples fürs
  Hochfahren.
- Initialisiert `power_key_runtime`, das den PMIC-Power-Taste-Handler
  anhängt, der einen kurzen Druck zum Sperrbildschirm und einen langen
  Druck zur Shutdown-Bestätigung leitet.
- Startet die Auto-Sleep-Runtime, die `device_sleep_service` verdrahtet,
  IMU-Samples auf Inaktivität abfragt, den Auto-Sleep-Worker-Task besitzt
  und Display-Sleep-/Light-Sleep-Aktionen behandelt.
- Initialisiert `timezone_service`, das Zeitzonen-/Zeit-Sync-Status aus
  NVS lädt, die konfigurierte Zeitzone anwendet und die Systemzeit aus
  der PCF85063-RTC vorbelegt, wenn verfügbar.
- Initialisiert `wifi_service`, das gespeicherte WLAN-Zugangsdaten oder
  eingebaute sdkconfig-Zugangsdaten lädt, den Stationsmodus startet, wenn
  Zugangsdaten vorhanden sind, oder den AP-Einrichtungsmodus startet,
  wenn keine verfügbar sind.
- Initialisiert `recording_service` und loggt den Aufnahmestatus.
- Initialisiert `recording_session_service`, das den
  Drücken/Halten-Aufnahme-Ablauf, die Tag-Auswahl und die
  Transkriptions-/Speichern-Orchestrierung besitzt.
- Initialisiert `footer_runtime`, das das Fußzeilen-Layout
  (Einstellungen-, WLAN-, Zeit-, Ordner-, Sticky-, Home-Buttons — in
  dieser Links-nach-rechts-Reihenfolge — plus den Mikrofon-Status) und
  das Fußzeilen-Fokus-Projektionsmodell vorbelegt. Der Sticky-Button
  (links von Home) öffnet das Follow-up-Sticky-Note-Overlay.
- Initialisiert `button_service`.
- Abonniert Tasten- und Touch-Ereignisse, leitet Nutzer-Aktivität an
  Auto-Sleep weiter, leitet Interaktions-Feedback an `feedback_service`
  weiter und behandelt Tasten-ausgelöste Sperrbildschirm-, Refresh- und
  Shutdown-Intents.
- Abonniert WLAN-Ereignisse und leitet den Verbindungsstatus an
  `timezone_service` weiter, damit der Netzwerk-Zeit-Sync startet,
  sobald Stationsverbindung verfügbar ist.
- Führt einen kleinen Shutdown-Task aus, damit Tasten-Callbacks einen
  Shutdown anfordern können, ohne direkt die PMIC-Abschalt-Sequenz
  auszuführen.
- Belegt Statusleisten- und Fußzeilen-Status vor (kein Refresh), zeichnet
  dann den ersten Bildschirm mit einem einzigen **vollständigen**
  Refresh als Erstes, das das Panel malt, und setzt schließlich
  `s_startup_complete`. Beim ersten Boot (NVS
  `app_state`/`onboarded` nicht gesetzt) ist dieser erste Bildschirm die
  Onboarding-Seite (`ShowOnboardingScreen(kFull)`); sonst ist es das
  Dashboard (`ShowHomeScreen(kFull)`).

**Boot-Refresh-Policy — kein Partial-Refresh vor dem ersten vollständigen
Anstrich.** Das Erste, was das Panel beim Booten malt, ist dieser eine
**vollständige** Refresh des ersten Bildschirms (Onboarding oder Home,
der letzte Schritt oben). Kein Partial-Refresh davor erlaubt. Dienste
initialisieren sich *vor* diesem Anstrich und veröffentlichen mehrere
Ereignisse während des Bootens (der RTC-Zeit-Intent, WLAN-
Verbindungsstatus, der Aufnahme-Archiv-Snapshot, Speicher-Mount); ihre
Handler aktualisieren UI-Status, dürfen aber noch **keinen** Refresh
anfordern, weil der kommende vollständige Anstrich ohnehin jede Surface
neu zeichnet — ein früherer Partial ist redundante Arbeit, und ein
Partial auf einem frisch eingeschalteten Panel ohne vollständige
Flush-Basislinie geistert. Die Regel wird durchgesetzt, indem jede
Handler-Refresh-Anfrage auf `s_startup_complete` gegated wird, eingefaltet
in das `ScreenActiveForRefresh(screen)`-Prädikat (`s_startup_complete &&
GetCurrentScreen() == screen`); die Statusleisten- und Fußzeilen-Pfade
prüfen `s_startup_complete` direkt. Alles Neue, das auf ein
Dienst-Ereignis neu zeichnet, muss durch dasselbe Gate laufen, damit es
still bleibt, bis der erste vollständige Anstrich landet. Nach dem Boot
reduziert sich das Prädikat auf "ist dieser Bildschirm aktiv", sodass
Live-Updates normal per Partial-Refresh laufen.

Aktuelle App-Ebene-Tasten-Interaktionen sind:

- `HOCH` / `RUNTER` bewegen den umlaufenden Fokus (oder scrollen ein
  betretenes Steuerelement) einen Schritt pro Druck, mit Umlauf, auf dem
  aktiven Bildschirm. Navigation wird beim Drücken (press-down)
  ausgelöst; ein reiner `HOCH`/`RUNTER`-Einzelklick (das
  Loslass-Ereignis) ist wirkungslos.
- `BOOT` und die mittlere Wipptaste (`FN`) klicken beide einfach, um das
  fokussierte Element auf dem aktiven Bildschirm zu aktivieren/
  abzusenden (Fußzeilen-Ziel, Seiten-Steuerelement oder Modal-Aktion).
  `button_service::IsPrimaryButton` macht die beiden gleichwertig.
- `RUNTER` drücken und **halten** (ein langer Druck) ist die
  app-weite "ein betretenes Steuerelement verlassen"-Geste, pro
  Bildschirm behandelt: sie tritt aus einem Steuerelement heraus, in das
  der Nutzer eingestiegen ist — z. B. die Vibe-Check-Karte, ein
  betretener Scroll-Container/eine Timeline-Elementliste auf den
  Seiten Zusammenfassen/Notizen/Todos/Follow-up, die WLAN-Netzwerkliste
  oder das Sticky-Note-Transkript-Scrollen. Auf App-Ebene ist das ein
  No-op. (Das löst den früheren `RUNTER`-Doppelklick-Ausstieg ab.)
- Ein kurzer Druck der `PWR`-Taste schaltet den Sperrbildschirm um; ein
  ~1s-Halten öffnet das Shutdown-Bestätigungs-Modal. `PWR` ist keine
  GPIO-Taste: beide kommen als AXP2101-Interrupts an, dekodiert von
  `main/power_key_runtime`. Ein anhaltendes 6s-Halten umgeht die
  Firmware komplett, der PMIC kappt dann die Rails.
- Die mittlere Wipptaste hat keine Doppelklick- oder Lang-Druck-Aktion.
  Sperren und Shutdown zogen zu `PWR` um; Aufnahme ist exklusiv `BOOT`
  vorbehalten.
- `BOOT` drücken und halten bewaffnet und startet dann den
  Aufnahme-Session-Ablauf; Loslassen stoppt ihn. Siehe
  [Aufnahme-Ablauf](#aufnahme-ablauf) für das, was zwischen dem
  Loslassen und dem Tag-Menü passiert.
- während das Auswahl-Modal sichtbar ist, bewegen `HOCH` und `RUNTER`
  beim Drücken plus gategatetem Halte-Wiederholen den umlaufenden Fokus
  mit Umlauf, ein Primärtasten-Klick sendet das fokussierte Element ab,
  und Touch fokussiert das berührte Element bei Kontakt, bevor beim
  Loslassen abgesendet wird.
- während das Shutdown-Modal sichtbar ist, bewegen `HOCH` und `RUNTER`
  beim Drücken plus gategatetem Halte-Wiederholen den umlaufenden Fokus
  mit Umlauf, ein Primärtasten-Klick aktiviert die fokussierte Aktion,
  und Touch fokussiert `Abbrechen` oder `Herunterfahren` bei Kontakt,
  bevor beim Loslassen aktiviert wird.
- wenn kein Overlay die Eingabe abfängt, nehmen Fußzeilen-Ziele am
  selben Touch-Modell teil: Touch-Down fokussiert das Fußzeilen-Element
  sofort, und Touch-Up aktiviert das bewaffnete Fußzeilen-Ziel. Auf
  seiteneigenen Bildschirmen wird dieser Touch-Down-Fokus direkt in den
  seitenlokalen Fokus-Wahrheitswert übersetzt, bevor die
  Fußzeilen-Projektion neu gezeichnet wird.

Shutdown läuft weiterhin über den zurückgestellten AppShell-Shutdown-
Task, damit die PMIC-Abschalt-Sequenz nicht innerhalb des
Tasten-Callbacks ausgeführt wird. Der Shutdown-Akkord läuft jetzt zuerst
über den App-eigenen Eingabe-/Overlay-Pfad, und nur eine bestätigte
Modal-Aktion benachrichtigt den Task. Der Task wartet kurz, bevor er
`power_service::RequestShutdown()` aufruft, damit der analoge
Tasten-/Q2-Bootstrap-Pfad Zeit hat, das Speisen von `PWR_EN` zu stoppen.

Aktuelle Eingabe-Priorität und Fokus-Besitz sind:

- Overlay-umlaufender/Hit-Fokus zuerst: Karten-Modal, Auswahl-Modal,
  Tastatur und das Vollseiten-Sticky-Note-Overlay, plus der
  schließbare Toast
- Fußzeilen-Ziele, wenn kein Overlay die Eingabe abfängt
- registrierte Seiten-Ziele nach der Fußzeile, unter dem gemeinsamen
  Seiten-Touch-Vertrag

Aktuelles Fokus-Surface-Inventar ist:

- `Home`: die Dashboard-Seite (fokussierbares Menü) plus die Fußzeile
- `Onboarding`: die Karussell-Seite (Schließen-/Zurück-/Weiter-
  Steuerelemente); keine Fußzeile
- `VibeCheck`, `Summarize`: gemeinsamer Seiten-Fokus-Pfad
- `Notes`, `Todos`, `FollowUp`: gemeinsamer Seiten-Fokus-Pfad mit einer
  zweistufigen Timeline (Datums-Gruppen-Chips → betretene Elementliste)
- `Details`: gemeinsamer Seiten-Fokus-Pfad mit einem betretenen
  Transkript-Scroll-Container
- `Settings`: gemeinsamer Seiten-Fokus-Pfad (inkl. dem "Manuell"-
  Onboarding-Button)
- `WiFi`: gemeinsamer Seiten-Fokus-Pfad, mit seiteneigenem
  Listen-Unterfokus für Netzwerke
- `Time`: gemeinsamer Seiten-Fokus-Pfad, mit Overlay-Editoren
  (Zeitzonen-Auswahl-Modal, numerische Tastatur pro Feld)
- `Sperrbildschirm`: heute keine fokussierbare Seiten- oder
  Fußzeilen-Surface
- Karten-Modal-/Auswahl-Modal-/Tastatur-/Toast-/Sticky-Note-Overlays:
  Overlay-Pfad

Der Besitz ist bewusst so aufgeteilt:

- `components/page_navigation/roving_focus`: wiederverwendbare
  Umlauf-Index-Primitive ohne eingebauten Modal-, Display- oder
  App-Shell-Besitz
- `components/page_navigation/navigation_input_controller.*`: gemeinsame
  Press-Erzeugung und Halte-Wiederholen-Gating für Navigationstasten
- `main/input_callback_dispatcher.*`: dedizierter latest-wins-
  Eingabe-Callback-Task für App-eigenes Tasten-Routing
- `main/input_runtime_setup.*`: App-eigene Roh-Tasten-/Touch-Bindungs-
  Einrichtung plus ein gemeinsames Eingaben-aktiviert-Gate vor Beginn des
  App-Routings
- `main/button_input_runtime.*`: app-weite Hardware-Tasten-Dispatch-
  Policy, die rohe Tasten-Ereignisse in gemeinsames Navigations-
  Press-/Halte-Verhalten übersetzt, bevor sie Seiten- oder
  Overlay-Code erreichen
- `main/input_focus_runtime.cpp`: App-eigenes Fokus-Routing,
  Touch-Kontakt-Status und app-weite Priorität für Overlay-,
  Fußzeilen- und Seiten-Ziele
- `main/page_input_runtime.*`: aktives Seiten-Eingabe-Routing für
  aktuell seiteneigene Bildschirme, damit `app_shell` und
  `input_focus_runtime` Seiten-Verhalten nicht fest verdrahten, und
  damit neutrale Seiten-Interaktions-Ergebnisse an einer Stelle
  angewendet werden statt in einzelnen Seiten-Runtimes
- `main/settings_page_interactions.*`, `main/wifi_page_interactions.*`
  und `main/time_page_interactions.*`: fokussierte Interaktions-Helfer,
  die aktuellen Seiten-Fokus in neutrale Seiten-Ergebnisse plus
  Folge-Intents übersetzen, während Dienst-Effekte und
  Orchestrierungs-Callbacks außerhalb des Coordinators bleiben
- `main/page_interaction_runtime.cpp`: Registrierungspunkt, über den
  künftige Seiten-Runtimes/Coordinators
  `resolve -> focus -> activate`-Touch-Hooks bereitstellen
- `main/overlay_runtime.cpp`: persistenter Overlay-Status,
  Fokus-Sync, Absenden- und Verwerfen-Verhalten für Karten-Modal,
  Auswahl-Modal, Tastatur, Toast und Sticky-Note-Overlays
- `main/footer_runtime.cpp`: rein präsentations-seitige Projektion von
  Fußzeilen-Layout und geteiltem Seiten-Fokus in den E-Paper-
  Fußzeilen-Vertrag, plus Fußzeilen-Touch-resolve-/focus-/activate-Hooks
  für fußzeileneigene Surfaces wie `Home`
- `main/app_shell.cpp`: nur Orchestrierung; verdrahtet Tasten-/
  Touch-Ereignisse in die fokussierten Runtime-Helfer und komponiert
  übergeordnete Produkt-Policy

Der aktuelle gemeinsame Seiten-Touch-Vertrag für aktuelle und künftige
seiteneigene Bildschirme ist:

- `resolve_touch_target(x, y, target)`: feststellen, ob ein
  seiteneigenes interaktives Ziel berührt wurde
- `focus_touch_target(target)`: seiteneigenen Fokus-Wahrheitswert sofort
  aktualisieren
- `activate_touch_target(target)`: seiteneigene Aktivierung bei
  Touch-Loslassen ausführen

Dieser Vertrag ist von jedem seiteneigenen Bildschirm implementiert:
`Dashboard` (Home), `Onboarding`, `VibeCheck`, `Summarize`, `Notes`,
`Todos`, `FollowUp`, `Details`, `Settings`, `WiFi` und `Time`. Das
Dispatching für den aktiven Bildschirm ist zentralisiert in
`main/page_input_runtime.cpp` (`resolve/focus/activate` und
Tasten-Behandlung pro `ScreenId`).

Künftige Seiten sollten seitenlokale ausgewählte Indizes als
Render-Projektionen des seiteneigenen Fokus-Wahrheitswerts halten statt
einen zweiten, nur-touch-basierten Auswahl-Status zu erfinden.
Zusammengesetzte Seiten-Steuerelemente sollten sich in denselben
Vertrag einklinken statt einen zweiten Touch-Interaktionspfad
hinzuzufügen.

Seiteneigene Bildschirme besitzen auch den Fußzeilen-Fokus-Wahrheitswert,
immer wenn ihre Fußzeilen-Buttons Teil desselben Navigationsmodells sind.
Touch-Down auf `Settings`- oder `WiFi`-Fußzeilen-Zielen wird zuerst in
den Fokus-Index des Seiten-Coordinators übersetzt, dann wird die
Fußzeile als Projektion dieses seitenlokalen Status neu gezeichnet. Die
Fußzeilen-Runtime behält eigenständigen Fokus-Besitz nur bei
fußzeileneigenen Surfaces wie `Home`.

Gemeinsame Tasten-Navigations-Regeln sind:

- Navigations-Timing ist nicht seiteneigen
- Navigations-`press down` und gategatetes Halte-Wiederholen-Verhalten
  laufen zuerst über die gemeinsame Eingabe-Runtime
- gemeinsames Halte-Wiederholen nutzt ein explizites
  Erst-Wiederholen-Gate vor intervallbasierten Wiederholungen, damit das
  Timing stabil bleibt, auch wenn rohe Wiederholungs-Callbacks ruckeln
- auf der aktiven WLAN-Netzwerkliste nutzt Halte-Wiederholen Seiten-
  Sprünge, dimensioniert nach der aktuell sichtbaren Zeilen-Kapazität,
  während press-down weiterhin um eine Zeile vorrückt
- veraltete, wartende Navigations-Callbacks sollten durch den neuesten
  Callback für dieselbe Tasten-Spur ersetzt werden
- Seiten-Module besitzen nur `MoveFocus(...)`, Aktivierungs-Semantik und
  den Aufbau von persistentem Seiten-Status

Der aktuelle app-weite Touch-Lebenszyklus ist:

- Touch-Down löst das höchstpriore Ziel auf und fokussiert es sofort
- Touch-Move kann den Fokus umlenken, solange der Kontakt aktiv bleibt
- Touch-Up aktiviert nur das aus diesem Kontakt bewaffnete Ziel
- Touch-Up ohne bewaffnetes Ziel bricht die Aktivierung ab, ohne einen
  zweiten Auswahl-Status zu erfinden

Der aktuelle app-weite Interaktions-Feedback-Lebenszyklus ist:

- Seiten-, Fußzeilen-, Overlay- und Eingabe-Fokus-Helfer dürfen
  entscheiden, dass eine Interaktion Produkt-Feedback erzeugen soll,
  sollten aber nur neutrale, App-eigene Feedback-Cues auslösen
- gemeinsame Interaktions-Verträge wie `main/app_interaction_result.h`
  sollten app-eigene Cue-Enums nutzen statt direkt von
  `feedback_service`- oder `system_sound_service`-Typen abzuhängen
- persistenter Overlay-Status darf einen anstehenden neutralen
  Feedback-Cue einreihen, wenn sich die Modal- oder Toast-Präsentation
  ändert, sollte aber nicht direkt Sound-Feedback abspielen
- `main/app_shell.cpp` ist die einzige Stelle, die neutrale, app-eigene
  Feedback-Cues auf `feedback_service`-Ereignisse abbildet und die
  tatsächliche Wiedergabe anfordert

Das hält den Interaktions-Besitz lokal, während es verhindert, dass
Feedback-Policy in wiederverwendbare Runtime-Helfer oder gemeinsame
Interaktions-Verträge einsickert.

### Zeit-Einstellungs-Seite

Die Zeit-Einstellungs-Seite ist der erste Bildschirm, der End-zu-Ende
durch das vollständige Seiten-Muster portiert wurde, und ist das
Referenz-Beispiel für das Hinzufügen einer Seite. Sie wird über den
globalen Fußzeilen-`Time`-Button erreicht und ist über fünf Schichten
zusammengesetzt:

- **View-Renderer** (`components/epaper_ui/time_page.*`): ein
  zustandsloses `DrawTimePage` plus `TimePageState`, Grenzen und
  Hit-Test. Wie die anderen Seiten-Renderer lebt er in `epaper_ui`
  (nicht `main/`), weil `display_service` — eine Komponente — ihn
  zeichnet und nicht von `main` abhängen kann. Er komponiert die
  Primitiven der Seite: `select_input` (Zeitzone), `time_input`
  (Stunde/Minute/Monat/Tag/Jahr) und `button` (AM-PM und
  Synchronisieren & Speichern). `text_input` ist die gemeinsame
  Feld-Primitive, auf der diese aufbauen und die `password_input`
  jetzt umschließt.
- **Coordinator** (`main/time_page_coordinator.*`): besitzt die
  editierbaren Feldwerte, das Navigationsmodell plus umlaufenden Fokus,
  lädt Status aus `timezone_service` und baut `TimePageState` und den
  Speicher-Patch. Ein `user_edited_`-Guard verhindert, dass
  Hintergrund-Uhr-Ereignisse laufende Bearbeitungen überschreiben;
  `MarkSaved()` löscht ihn nach einem Speichern, damit ein späterer Sync
  die Seite neu lädt.
- **Interactions** (`main/time_page_interactions.*`): bilden das
  fokussierte Steuerelement auf einen neutralen Aktivierungs-Intent ab
  (Zeitzonen-Modal öffnen, ein numerisches Feld bearbeiten, AM/PM
  umschalten, speichern oder Fußzeilen-Navigation), ohne Nebenwirkungen.
- **Runtime** (`main/time_page_runtime.*`): der mutex-gesicherte
  Orchestrator — Fokus-Bewegung, Touch-resolve-/focus-/activate,
  Fußzeilen-Projektion, die Overlay-Editoren und der Speichern-Ablauf.
  Status wird an `display_service` geschoben, und Refreshes werden über
  `ui_refresh_runtime` (`SurfaceKey::kTimePage`) geplant.
- **Integration**: `display_service` erhält `ScreenId::kTime`,
  `SetTimePageState` und einen `ApplyTime`-/`DrawTimeUnderlay`-Pfad;
  `page_navigation` erhält den `kTime`-Scope, die Steuerelement-Rollen
  und `BuildTimePageNavigationModel`; `page_input_runtime` routet den
  Bildschirm; und `app_shell` stellt `ShowTimeScreen` plus den
  Fußzeilen-`Time`-Eintrag bereit.

Feld-Bearbeitung passiert in Overlays, sie erbt also die
Overlay-Refresh-Regel von oben:

- Das Zeitzonen-Steuerelement öffnet ein scrollbares `select_modal`
  über `timezone_service::ListTimezones()`; der gewählte Index wird
  über den Auswahl-Modal-Absenden-Hook der Runtime übernommen.
- Jedes numerische Feld öffnet die Tastatur in ihrem `kNumbers`-Layout
  — ein eigenständiges Wähltastenfeld (`1`-`9`, dann `Bksp | 0 |
  Fertig`); der eingetippte Wert wird beim Absenden übernommen.

Der Speicher-/Sync-Ablauf:

- `BuildSettingsPatch` wandelt die Felder um (12h + AM/PM zu 24h,
  `YYYY-MM-DD` und `HH:MM`), und `Save()` ruft
  `timezone_service::ApplySettingsPatch`, zeigt dann einen
  Ergebnis-Toast. Das interne `Notify` des Patches treibt
  `HandleTimezoneEvent`, das die Seite bereits neu synchronisiert, also
  läuft `Save()` keinen redundanten vollständigen Sync mehr.
- `ApplySettingsPatch` führt den blockierenden SNTP-Pfad nie auf dem
  Task des Aufrufers aus. Wenn das Netzwerk steht, reiht es den
  NTP-Sync auf dem dedizierten `timezone_sync`-Worker ein
  (`QueueSync`); das Ergebnis kommt asynchron über den SNTP-Callback
  -> `Notify` -> Ereignis zurück. SNTP inline auszuführen ließ den
  kleinen Touch-Task-Stack überlaufen.
- Die Uhr synchronisiert sich auch bei jeder WLAN-Neuverbindung neu:
  `SetNetworkConnected` reiht einen Sync beim Übergang von getrennt
  nach verbunden ein, immer wenn die Uhr aktiviert und eine Zeitzone
  gesetzt ist (Standard Eastern). Der Übergangs-Guard verhindert, dass
  wiederholte "verbunden"-Ereignisse NTP zuspammen.

Das `location`-Feld in den zugrundeliegenden `timezone_service`-
Einstellungen wird bewusst nicht auf dieser Seite angezeigt. Es ist nur
Metadaten (in NVS und im Web-Portal gehalten) und hat keine Wirkung auf
die Zeithaltung, die allein von der Zeitzonen-Auswahl plus NTP
gesteuert wird.

## Aufnahme-Ablauf

`recording_session_service` besitzt die gesamte Drücken-und-Halten-
Aufnahme, vom ersten Cue bis zum Tag-Menü. Die Phasen-Maschine ist:

```text
kIdle -> kArmed -> kStartCue -> kRecording -> kStopCue -> kPlayingBack
      -> kAwaitingTagSelection -> kSaving -> kTranscribing -> kComplete
```

- **kArmed** — `BOOT`-Press-Down bewaffnet den Recorder.
- **kStartCue** — die Halte-Schwelle löst aus, die Aufnahme startet, und
  der Start-Cue (`SoundCue::kSpeaking`) spielt. Die Aufnahme startet
  bewusst *vor* dem Cue: auf das Ende des Cues zu warten würde das
  erste Wort des Sprechers verschlucken, deshalb überlappt der Cue mit
  den ersten Momenten der Aufnahme.
- **kRecording** — betreten, wenn der Start-Cue fertig ist. Wird `BOOT`
  losgelassen, während noch `kStartCue` läuft, wird das Beenden
  zurückgestellt statt verworfen (`s_finish_pending_after_start_cue`);
  ohne das würde ein Halten, das kaum länger als der Cue dauert, nie
  stoppen.
- **kStopCue** — Loslassen beendet die Aufnahme und spielt den
  Stop-Cue (`SoundCue::kInterrupt`). Die Wiedergabe wartet, bis der Cue
  fertig ist, statt zu überlappen, da beide dieselbe eine
  Codec-Ausgabe teilen.
- **kPlayingBack** — die Aufnahme wird dem Nutzer aus den PSRAM-Chunks
  vorgespielt, über `playback_service::PlayClip`, auf einem
  kurzlebigen Worker-Task. Die Wiedergabe blockiert für die Länge des
  Clips, kann also nicht auf dem Cue-Callback-Task laufen.
- **kAwaitingTagSelection** — das Tag-Menü öffnet sich. **Der Clip liegt
  an diesem Punkt noch nur im PSRAM.** Das ist der Grund, warum die
  Wiedergabe zuerst kommt: der Nutzer hört die Aufnahme, und die
  `Verwerfen`-Option im Menü wirft eine schlechte Aufnahme weg, ohne
  dass sie je die SD-Karte erreicht. Das Speichern passiert in
  `kSaving`, nachdem ein Tag gewählt wurde.

Jeder Ausgang aus `kStopCue` läuft auf `AdvanceToTagSelection` zusammen —
Wiedergabe fertig, Wiedergabe konnte nicht starten, oder der Stop-Cue
selbst schlug fehl — sodass ein fehlender oder kaputter Cue zu "keine
Wiedergabe" degradiert, statt die Session steckenzulassen.

Cue-Callbacks tragen ein Token, das bei jedem eingereihten Cue und bei
`ResetToIdleLocked` erhöht wird, sodass ein Ergebnis, das nach einem
Abbruch ankommt, verworfen wird statt einen veralteten Übergang
auszulösen.

Auto-Sleep ist blockiert, während `playback_service::IsPlaying()` wahr
ist, da weder die Wiederholung noch die Play-Aktion der Details-Seite
den Aufnahme-Status berühren und der Inaktivitäts-Timer sonst
während des Clips weiterzählen würde.

## Task-Zuordnung

App-eigene FreeRTOS-Tasks nutzen die gemeinsame Zuordnung in
`components/task_config/include/followup_task_config.h`. Die App ist um
eine einfache Aufteilung herum optimiert:

- CPU0 ist die System-/Netzwerk-Seite. ESP-IDF lässt dort im aktuellen
  `sdkconfig` bereits den Main-Task, `esp_timer` und die WLAN-Treiber-
  Arbeit laufen, App-WLAN-/Zeit-Koordination bleibt also nah an dieser
  Seite.
- CPU1 ist die Produkt-Hardware-/UI-Seite. Touch, Audio-Aufnahme,
  Speicher-Arbeit, Sound-Feedback und Sleep-getriebene Display-Übergänge
  werden von CPU0 ferngehalten, während die App wächst.

Bei Single-Core-Builds bildet die gemeinsame Task-Konfiguration den
App-Core zurück auf CPU0 ab.

| Task | Besitzer | Priorität | Core | Verantwortung |
| --- | --- | ---: | --- | --- |
| `record_capture` | `recording_service` | 5 | CPU1 | Timing-sensitive Mikrofon-Aufnahme, Pre-Roll und Clip-Pufferung. |
| `axp2101_irq` | `axp2101` | 2 | CPU1 | PMIC-Interrupt-Bedienung, inklusive Power-Taste kurz/lang. |
| `app_sleep` | `device_sleep_runtime` | 4 | CPU1 | Display-Sleep, Light-Sleep-Ein-/Ausstieg und Aufwach-Wiederherstellungs-Aktionen. |
| `app_shutdown` | `app_shell` | 4 | CPU1 | Zurückgestellte PMIC-Abschaltung, nachdem das Shutdown-Modal bestätigt wurde. |
| `sleep_motion` | `device_sleep_runtime` | 3 | CPU1 | 200-ms-IMU-Abfrage und Bewegungs-/Stillstands-Klassifikation. |
| `wifi_transition` | `wifi_service` | 3 | CPU0 | WLAN-Station-/AP-/Stop-/Trenn-Übergänge. |
| `wifi_callbacks` | `wifi_service` | 3 | CPU0 | App-seitige WLAN-Ereignis-Zustellung außerhalb der ESP-Event-Callbacks. |
| `storage_service` | `storage_service` | 2 | CPU1 | Lang laufende SD-Operationen wie Formatieren. |
| `timezone_sync` | `timezone_service` | 2 | CPU0 | SNTP-Sync, System-Zeit-Update und RTC-Rückschreiben. |
| `clip_playback` | `recording_session_service` | 2 | CPU1 | Kurzlebiger Worker, der einen gerade aufgenommenen Clip abspielt. |

Die Zuordnung hält lang laufende SD-Arbeit bewusst unter Eingabe und
Audio-Aufnahme. Künftige Tasks sollten zuerst in `task_config`
hinzugefügt werden, mit einer kurzen Besitz-Begründung, statt lokale
Prioritäts-/Core-Literale zu nutzen.

Treiber-spezifische Verdrahtung sollte aus `main/` herausgehalten
werden; der App-Start sollte stattdessen Dienst-Ebene-APIs aufrufen.
Produkt-spezifische Sequenzierung gehört in `app_shell`, nicht in
wiederverwendbare Komponenten.

WLAN- und Zeit-Dienste folgen derselben Grenze:

- `wifi_service` besitzt `esp_netif`, die Registrierung der
  Standard-ESP-Event-Loop, `esp_wifi`-Moduswechsel,
  Stations-/AP-Konfiguration, NVS-Zugangsdaten-Speicherung,
  Netzwerk-Scans und den HTTP-Backend-Server, der während der
  AP-Einrichtung genutzt wird.
- `timezone_service` besitzt Zeitzonen-Katalog/-Aliase, persistierte
  Zeitzonen-Einstellungen, SNTP-Einrichtung, System-Zeit-Updates,
  PCF85063-RTC-Lesen/Schreiben über `power_service` und
  Backend-HTTP-Routen für Zeit-Einstellungen.
- `app_shell` verdrahtet die beiden Dienste zusammen, indem es
  WLAN-Verbindungs-Ereignisse an
  `timezone_service::SetNetworkConnected(...)` weiterleitet.

Runtime-persistierte Einstellungen leben in dienst-eigenen
NVS-Namespaces:

- `wifi`: `ssid`, `password`
- `timezone`: `enabled`, `tz_name`, `location`, `time_src`, `ntp_sync`,
  `ntp_epoch`

Die Build-Zeit-WLAN-/Zeit-Standardwerte leben unter `Folloup Settings`:

- `CONFIG_FOLLOWUP_WIFI_AP_PREFIX`
- `CONFIG_FOLLOWUP_WIFI_STA_SSID`
- `CONFIG_FOLLOWUP_WIFI_STA_PASSWORD`
- `CONFIG_FOLLOWUP_WIFI_START_IN_AP_MODE`
- `CONFIG_FOLLOWUP_TIME_SYNC_DEFAULT_ENABLED`
- `CONFIG_FOLLOWUP_DEFAULT_TIMEZONE_NAME`

Gespeicherte NVS-WLAN-Zugangsdaten haben Vorrang vor eingebauten
sdkconfig-Zugangsdaten. Existiert keines von beiden, oder ist
`CONFIG_FOLLOWUP_WIFI_START_IN_AP_MODE` aktiviert, geht `wifi_service`
in den offenen AP-Einrichtungsmodus und stellt Backend-Routen unter der
SoftAP-URL bereit, normalerweise `http://192.168.4.1`. Das aktuelle
Backend stellt absichtlich nur JSON-/Formular-Endpunkte bereit; es
bettet die alte Portal-UI nicht ein und fügt keine DNS-Captive-Portal-
Umleitung hinzu.

Aktuelle WLAN-Backend-Routen:

- `GET /`
- `GET /api/status`
- `GET /api/scan`
- `POST /api/configure`
- `POST /api/disconnect`

Aktuelle Zeit-Backend-Routen, registriert auf demselben HTTP-Server:

- `GET /api/settings/time`
- `PATCH /api/settings/time`
- `GET /api/runtime/time`
- `GET /api/timezone/list`

Auto-Sleep ist aufgeteilt auf eine Policy-Komponente und einen
Produkt-Runtime-Helfer: `device_sleep_service` besitzt Sleep-Status,
Timer, Timeout-Validierung, Blocker-Status und Übergangs-Ereignisse,
fasst aber weder Display, GPIO noch ESP-Sleep-Hardware an.
`main/device_sleep_runtime.cpp` besitzt produkt-spezifisches
Auto-Sleep-Runtime-Verhalten: IMU-Inaktivitäts-Abfrage, den
Event-Worker-Task, Display-Sleep-Befehle, ESP-Light-Sleep-Eintritt,
Aufwach-Behandlung und App-Ebene-Blocker-Aggregation. `app_shell` sollte
nur Einstellungen bereitstellen, App-eigene Signale wie
Shutdown-anstehend-Status liefern, die Runtime starten und Nutzer-
Aktivität weiterleiten. Siehe `docs/auto-sleep.md` für das stabile
Feature-Verhalten und den zurückgestellten
FIFO-/geteilter-Interrupt-Plan.

Aktuelles Auto-Sleep-Verhalten:

- `main/device_sleep_runtime.cpp` fragt `imu_service::ReadSample(...)`
  alle `200 ms` ab und rechnet Beschleunigungs-Deltas von `g` in `mg`
  um.
- Bewegung wird erkannt, wenn die Achsen-Delta-Summe mindestens
  `60 mg` beträgt oder das größte Achsen-Delta mindestens `25 mg`.
- Stillstand wird erst nach einem durchgehenden `2 s`-Fenster erkannt,
  in dem die Achsen-Delta-Summe höchstens `20 mg` und das größte
  Achsen-Delta höchstens `8 mg` beträgt.
- Display-Sleep aktualisiert das E-Paper-Panel auf einen leeren
  Bildschirm und legt das Panel dann schlafen.
- ESP32-S3-Light-Sleep wartet, bis `ACTION`/`GPIO0` losgelassen wird,
  bewaffnet dann `ACTION` und den PMIC-IRQ/`GPIO38` als
  active-low-`gpio_wakeup_enable`-Quellen. Es setzt das Tasten-Polling
  aus (Light Sleeps Uhr-Sprung würde sonst beim Aufwachen jeden
  verpassten Tick nachspielen und die Klick-Klassifikation zerstören),
  bewaffnet aufwach-nur-`ACTION`-Ereignis-Unterdrückung, damit der
  Aufwach-Druck keine Aufnahme starten kann, aktualisiert das Panel auf
  einen leeren Bildschirm, legt das Panel schlafen und betritt
  `esp_light_sleep_start()`. Es gibt keinen Power-Latch zu bewahren:
  der AXP2101 hält die Rails über den Sleep hinweg.
- Die aufwach-auslösenden Power-Taste-Ereignisse werden nach Light Sleep
  als aufwach-nur konsumiert, sie lösen also kein normales
  Power-Taste-Verhalten aus und setzen `shutdown_pending` nicht als
  Auto-Sleep-Blocker.
- Nach dem Light-Sleep-Aufwachen wird das Display mit einem erzwungenen
  vollständigen Refresh auf einen leeren Bildschirm zurückgesetzt.
- Inaktivität ist blockiert, während eine Aufnahme aktiv, bewaffnet,
  wird gespeichert oder exportiert; während ein Clip abgespielt wird;
  während ein Shutdown ansteht; während ein E-Paper-Refresh aktiv ist;
  während app-deklarierter Speicher-Schreibaktivität; während der
  AP-Einrichtungsmodus aktiv ist; und während der SNTP-Zeit-Sync läuft.
- Die aktuellen Werkbank-Standardwerte sind `10 s` für Display-Sleep
  und `30 s` für Light-Sleep. Produktions-Standardwerte sollten später
  erhöht werden, wenn das Produktverhalten nicht mehr auf der Werkbank
  eingestellt wird.

SD-Karten-Formatierung bleibt über `storage_service` erreichbar, aber
aktuell ruft kein Demo-Button-Pfad das auf. Eine künftige App-UI sollte
die Speicher-API über ihre eigene Aktions-/Controller-Schicht aufrufen.

### `components/axp2101`

Das ist der AXP2101-PMIC-Treiber. Auf diesem Board ist der PMIC nicht
optional: er speist jede Rail, ein fehlender oder nicht reagierender
Chip ist also per Design nicht wiederherstellbar, und der Konstruktor
bricht ab.

Aktueller Umfang:

- besitzt die DC1-/ALDO1-3-Rails, das Einzelzellen-Ladeprofil und die
  System-Abschalt-Spannung
- stellt Akku-Füllstand, Spannung, Temperatur, Ladezustand und
  VBUS-Präsenz bereit
- besitzt die Power-Taste-Timings: Druck-bis-Einschalten, die
  IRQ-Pegel-Zeit, die einen kurzen von einem langen Druck trennt, und
  den Hardware-Druck-bis-Ausschalten-Halte
- führt einen dedizierten IRQ-Task aus, der das Status-Register liest
  und löscht, dann ein dekodiertes `InterruptEvent` an einen
  registrierten Callback im Task-Kontext übergibt

Der Treiber bleibt app-unabhängig. Was ein Power-Taste-Druck
*bedeutet*, gehört zu `main/power_key_runtime`, nicht hierher.

### `components/pcf85063`

Das ist der PCF85063-RTC-Treiber. Er teilt sich den Sensor-I2C-Bus mit
dem PMIC und dem IMU.

Aktueller Umfang:

- Wanduhrzeit lesen und schreiben
- den RTC-Rückschreib-Pfad des Zeitzonen-Dienstes bedienen
- Zeitzonen-Policy und SNTP-Terminierung aus dem Treiber heraushalten

### `components/board`

Diese Komponente zentralisiert Waveshare-spezifischen Hardware-Zugriff.
Sie ist die einzige Stelle, die die Pin-Zuordnung dieses Boards kennt;
generische Treiber bleiben board-unabhängig und werden hier komponiert.

`waveshare_board_config.h` besitzt die Pin-Zuordnung:

- ACTION-/BOOT-Taste: `GPIO_NUM_0` (auch der Light-Sleep-Aufwach-Button)
- Wipptaste hoch: `GPIO_NUM_4`
- Wipptaste mitte/FN: `GPIO_NUM_5`
- Wipptaste runter: `GPIO_NUM_6`
- E-Paper (SSD1677) an einem eigenen SPI3-Bus: BUSY `GPIO_NUM_3`, DC
  `GPIO_NUM_9`, CS `GPIO_NUM_10`, SCK `GPIO_NUM_11`, MOSI `GPIO_NUM_12`,
  RST `GPIO_NUM_46`, kein MISO
- MicroSD über den SDMMC-Controller, 4-Bit: CLK `GPIO_NUM_16`, CMD
  `GPIO_NUM_17`, D0 `GPIO_NUM_15`, D1 `GPIO_NUM_7`, D2 `GPIO_NUM_8`, D3
  `GPIO_NUM_18`
- geteiltes Sensor-I2C: SDA `GPIO_NUM_41`, SCL `GPIO_NUM_42` — trägt
  den AXP2101-PMIC (`0x34`), den QMI8658-IMU, die PCF85063-RTC, die
  ES8311-Codec-Steuerschnittstelle und einen SHTC3, den nichts
  ansteuert
- PMIC-Interrupt: `GPIO_NUM_38`
- ES8311-Audio über I2S0: MCLK `GPIO_NUM_13`, BCLK `GPIO_NUM_14`, WS
  `GPIO_NUM_47`, DIN `GPIO_NUM_21`, DOUT `GPIO_NUM_48`
- NS4150B-Verstärker-Freigabe: `GPIO_NUM_39`
- Panel-Geometrie: 800 x 480

Auf diesem Board gibt es keinen Power-Latch und keinen geteilten
SPI-Bus zu arbitrieren: der AXP2101 hält die Rails, und das Panel
besitzt SPI3 allein. Beides waren bedeutende Quellen von
Sticky-Ära-Komplexität, die hier schlicht nicht zutreffen.

`waveshare_board.h/.cpp` besitzt:

- `waveshare_board::EnablePowerHold()` — fährt die AXP2101-Rails, das
  Ladegerät und das Power-Taste-Verhalten hoch, und ist das Erste, was
  `app_shell::Run()` aufruft
- `waveshare_board::GetPmic()` — die geteilte `Axp2101`-Instanz
- `waveshare_board::GetAudioCodec()` — die geteilte `Es8311Codec`-
  Instanz, beim ersten Gebrauch erzeugt, mit für ihre Lebensdauer
  aktivierter Ausgabe (und damit dem PA)
- `waveshare_board::EnsureSensorI2cBus(...)` — der eine geteilte
  I2C-Master-Bus

Audio läuft vollduplex auf einem einzigen 16-kHz-Takt für Aufnahme und
Wiedergabe, gewählt passend zur Aufnahme- und Gemini-Pipeline, damit
nirgends im Pfad ein Resampling nötig ist. Die Ausgabelautstärke wird
auf `WAVESHARE_AUDIO_OUTPUT_VOLUME` (voller Pegel) gesetzt, bevor die
Ausgabe aktiviert wird — der NS4150B in einen kleinen MX1.25-
Lautsprecher hat keinen Headroom zu verschenken, und der Codec-eigene
Standard liegt deutlich unter dem, was in der Hand hörbar ist.

### `components/power_service`

Diese Komponente ist die App-seitige Strom-Schicht. Sie komponiert die
`board`-Helfer mit dem AXP2101-PMIC und der PCF85063-RTC.

Aktuelle Verantwortlichkeiten:

- stellt `power_service::EnablePowerHold()` bereit, damit `main` die
  PMIC-Rails als erste Anwendungs-Aktion hochfahren kann
- stellt `power_service::ReadStatus(...)` bereit
- loggt einen diagnostischen Snapshot über
  `power_service::LogDebugStatus()`

Der aktuelle diagnostische Snapshot enthält:

- Dienst-Initialisierungsstatus
- Akku-Füllstand, -Spannung und -Temperatur vom PMIC-Fuel-Gauge
- Ladezustand und Vollladung-Erkennung
- VBUS-Präsenz und -Spannung
- PCF85063-Control-/Status-2-Bits für Alarm-/Timer-Flags und
  Interrupt-Freigaben

Der AXP2101 besitzt Rails, Laden und Akku-Telemetrie, es gibt also
keine Ladegerät-GPIOs zu konfigurieren und keinen Strom-Eingangs-ADC
zu erfassen — beides war Sticky-spezifisch und hat hier kein
Gegenstück.

`power_service::RequestShutdown()` ist der App-seitige Shutdown-
Einstiegspunkt, erreicht über das Shutdown-Bestätigungs-Modal, das ein
langer `PWR`-Druck öffnet. Er löscht und deaktiviert zuerst die
PCF85063-Alarm-/Timer-Interrupt-Quellen, ruft dann
`Axp2101::PowerOff()` auf, das jede Rail kappt. Auf Akku wird das Board
dabei dunkel und kehrt nie zurück; solange VBUS anliegt, speist der
PMIC die Rail weiter, der Aufruf kann also mit weiterhin
eingeschaltetem Board zurückkehren — das Log sagt das auch so, und das
Abziehen von USB schließt das Abschalten ab.

### `components/button_service`

Diese C++-Komponente besitzt die App-seitige Tasten-Initialisierung und
das Logging. Sie nutzt Espressifs verwaltete `espressif/button`-
Komponente für die zugrundeliegende Entprellung und die
Tasten-Ereignis-Zustandsmaschine.

Aktueller Umfang:

- `ACTION`/BOOT auf `GPIO0`
- `HOCH` auf `GPIO5`
- `RUNTER` auf `GPIO6`
- active-low-GPIO-Tasten mit von der verwalteten Komponente aktivierten
  internen Pull-Widerständen
- loggt Press-Down, Press-Up, Einzelklick, Doppelklick, Lang-Druck-Start
  und Lang-Druck-Up
- stellt eine typisierte Ereignis-Callback-API für App-Ebene-Policy-
  Routing in `app_shell` bereit

Aktuelle App-Shell-Nutzung auf Basis dieser Low-Level-Ereignisse:

- `HOCH`/`RUNTER` Press-Down: bewegt umlaufenden Fokus (mit Umlauf), ein
  Schritt pro Druck. Ein reiner `HOCH`/`RUNTER`-Einzelklick (das
  Loslassen) ist wirkungslos.
- `BOOT` oder Wipptaste-Mitte `FN` Einzelklick: aktiviert/sendet das
  fokussierte Element ab
- `RUNTER` halten (Lang-Druck): app-weite "ein betretenes
  Steuerelement verlassen"-Geste, pro Bildschirm behandelt (auf
  App-Ebene ein No-op; löst den früheren `RUNTER`-Doppelklick ab)
- kurzer `PWR`-Druck: schaltet den Sperrbildschirm um (ein
  AXP2101-Interrupt, keine GPIO-Taste)
- ~1s-`PWR`-Halten: öffnet das Shutdown-Bestätigungs-Modal
- `BOOT` halten: bewaffnet/startet/beendet den Aufnahme-Session-Ablauf
- Auswahl-Modal sichtbar: `HOCH` und `RUNTER` bewegen beim Drücken plus
  gategatetem Halte-Wiederholen den gemeinsamen umlaufenden Fokus, und
  ein Primärtasten-Klick sendet ab
- Shutdown-Modal sichtbar: `HOCH` und `RUNTER` bewegen beim Drücken
  plus gategatetem Halte-Wiederholen den gemeinsamen umlaufenden
  Fokus, und ein Primärtasten-Klick aktiviert die fokussierte Aktion

Die App-Shell besitzt das Modal-Fokus-Routing nicht direkt. Sie gibt
Tasten-Ereignisse an `main/input_focus_runtime.cpp` weiter, das
Overlay-Fokus-Fallen zuerst die Chance gibt, Navigations-Bewegung zu
konsumieren, und dann Absenden-/Verwerfen-Arbeit an
`main/overlay_runtime.cpp` delegiert. `overlay_runtime` hält das Modal
über sowohl dem Home-Bildschirm als auch dem Sperrbildschirm und gibt
erst nach expliziter Bestätigung einen `request_shutdown`-Intent
zurück.

Die Auto-Sleep-Runtime bewaffnet `ACTION`/`GPIO0` und den
PMIC-IRQ/`GPIO38` als Light-Sleep-GPIO-Aufwach-Quellen. Die verwaltete
Button-Komponente besitzt weiterhin die normale Wachzustand-Entprellung
und Ereignis-Erzeugung; die Light-Sleep-Aufwach-Einrichtung bleibt in
`main/device_sleep_runtime.cpp`, damit Vor-Sleep-Aufwach-nur-Ereignis-
Unterdrückung, Tasten-Poller-Aussetzen/Fortsetzen und sofortige
Display-Wiederherstellung Teil der Auto-Sleep-Policy bleiben.

### `components/audio_hal`

Das ist die ES8311-Codec-Schicht. Sie löst den nur-PDM-Eingang-Pfad des
Sticky ab: dieses Board hat sowohl ein Mikrofon als auch einen
Lautsprecher (ES8311 plus ein NS4150B-Leistungsverstärker), der Codec
ist also vollduplex.

Aktueller Umfang:

- konfiguriert I2S0 für vollduplex 16-kHz-Mono-16-Bit-PCM, passend zur
  Aufnahme- und Gemini-Pipeline, damit nichts resampeln muss
- besitzt den NS4150B-Leistungsverstärker-Freigabe-Pin neben der
  Codec-Ausgabe-Freigabe
- stellt `OutputData(...)` für die Wiedergabe und die Aufnahme-Seite
  für die Aufnahme bereit
- stellt Ausgabe-Lautstärkeregelung bereit

Das Board hält die Codec-Ausgabe (und damit den PA) für die Lebensdauer
des Codecs aktiviert. Pro-Ereignis-PA-Umschalten wurde verworfen: Cues
und Clip-Wiedergabe teilen sich die Ausgabe, und Umschalten schnitt
jeweils den Stream ab, der als Zweiter startete.

### `components/system_sound_service`

Diese Komponente besitzt den dekodierten Sound-Cue-Katalog und streamt
Cues an den Codec.

Aktueller Umfang:

- dekodiert und cacht das eingebaute Cue-Set
- spielt einen Cue ab, optional mit einem Abschluss-Callback, der
  meldet, ob der Cue fertig wurde, entprellt, ersetzt, unterbrochen
  oder fehlgeschlagen ist
- serialisiert die Cue-Wiedergabe, damit sich zwei Cues nicht auf der
  Ausgabe überlappen können

Der Abschluss-Callback ist das, was `recording_session_service` erlaubt,
den Stop-Cue und die Review-Wiedergabe zu sequenzieren, ohne dass sie
sich überlappen.

### `components/feedback_service`

Diese C++-Komponente besitzt die App-seitige haptische/akustische
Feedback-Policy. Sie bildet Produkt-Ereignisse auf Sound-Cues ab, ohne
Codec-Details gegenüber `app_shell` offenzulegen.

Aktueller Umfang:

- initialisiert `system_sound_service` mit dem Audio-Codec des Boards
- bildet Start-, Sperr-, Entsperr-, Tasten-Klick-, Tasten-Doppelklick-,
  Tasten-Lang-Druck-, Shutdown- und Fehler-Feedback auf Sound-Cues ab
- hält App-Ebene-Feedback-Namen getrennt von Low-Level-Cue-Namen

`feedback_service` ist bewusst eine App-Shell-Abhängigkeit, keine
Runtime-Helfer-Abhängigkeit. App-eigene Helfer unter `main/` wie
`overlay_runtime`, `input_focus_runtime`, `footer_runtime` und künftige
Seiten-Runtimes sollten `feedback_service` nicht direkt aufrufen. Sie
sollten neutrale, App-eigene Feedback-Cues auslösen und `app_shell`
diese Cues auf `feedback_service`-Ereignisse abbilden lassen.

`app_shell` darf Feedback-Ereignisse für Tasten-Einzelklick, Tasten-
Doppelklick, Nicht-Strom-Lang-Druck-Start, Sperren, Entsperren,
Touch-Kontakt, Modal-Öffnen, Start, Shutdown und andere Produkt-Ebene-
Interaktions-Ergebnisse anfordern. Es sollte nichts über LEDC-Timer-
Nummern, PWM-Duty-Werte, GPIO-Einrichtung oder die genaue Sound-Cue-
Katalog-Zusammensetzung wissen.

### `components/sd_card`

Das ist der von hier portierte SDSPI-/FATFS-MicroSD-Wrapper:

```text
/Users/tieuvong/Desktop/folloup/sticky_port/Device_Peripheral_Demo/components/sd_card
```

Die Komponente ist größtenteils board-unabhängig. Sie erhält eine
`SdCardPins`-Struktur und einen Mount-Punkt von ihrem Aufrufer und
besitzt dann:

- SD-Power-Freigabe-GPIO-Konfiguration
- Karten-Erkennungs-GPIO-Konfiguration
- SDSPI-Bus-/Geräte-Einrichtung, außer der Aufrufer markiert den
  SPI-Bus als extern besessen
- FATFS-Mount/Unmount am angeforderten Mount-Punkt
- Speicher-Statistiken
- Verzeichnis-Auflistung
- kleine Datei-Lese-/Schreib-/Anhänge-/Trunkier-Helfer

Diese Komponente nicht von `board` abhängig machen; Pins vom Dienst
oder der Board-Schicht hereinreichen. Auf Sticky bittet
`storage_service` die Board-Schicht, zuerst den geteilten SPI-Bus zu
initialisieren, und übergibt `external_spi_bus=true`, sodass der
SD-Wrapper sein SDSPI-Gerät nur zum bestehenden Bus hinzufügt.

### `components/storage_service`

Das ist die App-seitige Speicher-Schicht. Sie komponiert `board`-Pin-
Definitionen mit dem `sd_card`-Wrapper und besitzt den App-SD-Karten-
Mount- und Format-Status.

Aktueller Umfang:

- nutzt die schematische Seite-5-MicroSD-Pin-Zuordnung
- prüft `SD_DETECT`
- mountet `/sdcard` während des Boots, wenn eine Karte steckt
- hält die Karte während des normalen Betriebs nach erfolgreicher
  Boot-Zeit-Initialisierung gemountet
- formatiert die SD-Karte auf Anfrage, setzt das `FOLLOUP`-Volume-Label
  und erstellt das Quell-App-Verzeichnis-Layout neu:
  `/recordings`, `/todos`, `/summaries`, `/files`, `/trash`,
  `/trash/recordings` und `/trash/todos`
- veröffentlicht nur groben Format-Lebenszyklus-Status: gestartet,
  erfolgreich oder fehlgeschlagen
- vermeidet Fortschritts-Checkpoint-UI-Unruhe während des Formatierens;
  der aktuelle Produkt-Ablauf zeigt ein einziges "Formatiere. Bitte
  warten..."-Modal, bis die Operation mit Erfolg oder Fehler
  abgeschlossen ist
- loggt Mount-Status, Gesamt-/Freie Bytes und eine kleine
  Root-Verzeichnis-Vorschau
- schreibt/liest `/sdcard/SDPROBE.TXT` einmal als Hochfahr-Probe

Eine fehlende SD-Karte ist kein fataler App-Startfehler. Mount-
Fehlschläge werden geloggt und als nicht-fatale Dienst-Initialisierungs-
Fehlschläge an AppShell zurückgegeben.

MicroSD teilt sich SPI-Leitungen mit dem E-Paper-Pfad:

- `SD_CLK/SCK` / `EP_SCK`: `GPIO13`
- `SD_CMD/MOSI` / `EP_SDI`: `GPIO14`
- `SD_D0/MISO`: `GPIO12`

Auf diesem Board nutzt die SD-Karte den SDMMC-Controller und das Panel
besitzt SPI3, es gibt also keinen geteilten Bus zu arbitrieren und
keinen Bus-Guard zu erwerben. Die geteilte-SPI-Reihenfolge-Regeln des
Sticky gelten hier nicht.

Während des Runtime-SD-Formatierens auf Sticky sollte der Speicher-Pfad
geteilte-SPI-Display-Aktivität minimieren. Die aktuelle Produkt-Policy
ist, das Formatierungs-Overlay einmal beim Formatierungsbeginn zu
zeigen und dann den SD-Worker ungestört zu lassen, bis am Ende das
Erfolgs- oder Fehler-Modal gezeigt wird. Zwischenzeitliche Format-
Fortschritts-Overlay-Refreshes nicht wieder einführen, außer ein
hardware-validierter Bedarf überwiegt das zusätzliche
Bus-Kontentions-Risiko.

Die Hardware-Validierung auf Sticky zeigte eine zusätzliche
board-spezifische Einschränkung: wenn eine SD-Karte steckt, muss die
Karte auf dem geteilten SPI-Bus initialisiert werden, bevor das
E-Paper-Panel diesen Bus zu nutzen beginnt, und die Karte sollte danach
gemountet bleiben. Die Karte nach dem Boot wieder abzubauen ließ das
Panel einen Refresh loggen, ohne den Bildschirm sichtbar zu
aktualisieren. "Erst SD, dann Display, und SD gemountet halten" als
notwendige Start-Policy auf dieser Hardware-Revision behandeln.

### `components/playback_service`

Diese Komponente streamt einen Clip an den Codec. Sie ist bewusst dumm:
keine Policy, kein Task-Besitz, kein Status außer "spiele ich gerade
ab".

Aktueller Umfang:

- `PlayFile(path)` streamt eine 16-kHz-Mono-16-Bit-PCM-WAV von SD
- `PlayClip(clip)` streamt die PSRAM-Chunks, die `recording_service`
  schon hält, ohne SD-Umweg
- `IsPlaying()`/`Stop()` für Aufrufer, die unterbrechen müssen

Beide Einstiegspunkte blockieren für die Länge des Clips, Aufrufer
führen sie also auf einem kurzlebigen Worker-Task aus. `PlayClip`
existiert für den Review-Schritt nach der Aufnahme: an dem Punkt wurde
die Aufnahme bewusst noch nicht auf SD geschrieben, ein vom Nutzer
verworfener Clip berührt die Karte also nie.

### `components/recording_service`

Das ist die App-seitige Sprach-Eingabe-Aufnahme-Schicht. Sie komponiert
die `audio_hal`-Codec-Aufnahmeseite mit App-Policy für Pre-Roll,
Aufnahme-Status, Clip-Besitz, Eingangspegel-Telemetrie und
WAV-Datei-Ausgabe.

Aktueller Umfang:

- erzeugt einen dedizierten Aufnahme-Task, der kurze PCM-Chunks vom
  Codec liest
- hält, solange bewaffnet, einen einsekündigen PSRAM-gestützten
  Pre-Roll-Ringpuffer
- unterstützt den Start einer Aufnahme mit oder ohne Pre-Roll
- speichert den aktiven Clip in PSRAM-gestützten Chunks mit maximal
  10 Sekunden Dauer
- verfolgt einen einfachen Eingangspegel-Prozentsatz für UI-/Debug-/
  VAD-Vorbereitung
- stellt `Arm()`, `Start()`, `Finish()`, `Cancel()`, `DiscardClip()` und
  `GetRecordedClip()` bereit
- speichert den letzten Clip als Mono-16-Bit-PCM-WAV-Datei auf der
  MicroSD

Der Dienst implementiert keine Wiedergabe. Er besitzt den Clip und gibt
ihn über `GetRecordedClip()` heraus; `playback_service` ist das, was
einen an den Codec streamt. Diese Trennung ist bewusst — die
Aufnahme-Schicht sollte keinen Ausgabe-Pfad bekommen, und
`playback_service` sollte nicht wissen, wie ein Clip entstand.

Künftige Sprach-Produkt-Arbeit sollte VAD, Upload/Transkription und
Anzeige-Status auf diesem Dienst aufbauen, statt diese Policies nach
`audio_hal` hinunterzudrücken.

### `components/epaper_panel`

Das ist der rohe Mono-SSD1677-E-Paper-Panel-Treiber, portiert von:

```text
/Users/tieuvong/Development/followup/components/board_drivers/epaper_panel
```

Der Treiber sollte board-unabhängig bleiben. Er erhält eine
`EpaperPanelConfig` von seinem Aufrufer und besitzt:

- E-Paper-Reset-, Busy-, Daten-/Befehls- und Chip-Select-GPIO-Steuerung
- den SSD1677-Befehls-/Daten-Schreibpfad
- den Mono-Framebuffer
- den zurückgehaltenen vorherigen Framebuffer (ein Schatten dessen, was
  auf dem Glas steht), genutzt vom Partial-Refresh-Differential
- vollständigen Basis-Refresh
- änderungs-erkannten Ganzbildschirm-Partial-Refresh (Framebuffer gegen
  den Schatten diffen, nur die geänderten Pixel ansteuern)
- Panel-Sleep
- Refresh-Timing-Metriken

Der aktuelle Umfang ist bewusst nur mono. Gray4-Unterstützung nicht
portieren, außer eine künftige Produkt-Anforderung verlangt das
ausdrücklich.

Das erste Display-Update muss `RefreshFullBase()` nutzen, damit die
aktuelle (`0x24`) und vorherige (`0x26`) RAM-Ebene des SSD1677 vorbelegt
wird. Pro-Interaktions-Updates rufen `RefreshChangedRegion()` auf, das
den frisch gerenderten Framebuffer gegen den zurückgehaltenen Schatten
diffed und, nur wenn sich etwas geändert hat, einen
Ganzbildschirm-Partial über `RefreshPartialFullScreen()` ansteuert. Das
Differential-Waveform bewegt physisch nur die Pixel, die sich
unterscheiden, ein Ganzbildschirm-Partial aktualisiert also weiterhin
nur das geänderte Element, ohne Blitzen. Wird ein Partial-Refresh
angefordert, bevor ein Basisbild existiert, nach Sleep/Timeout oder
nach dem Partial-Refresh-Limit, fällt der Treiber zurück auf
`RefreshFullBase()`.

> **Fenster-/Regions-Partial-Refresh wird auf diesem SSD1677
> (GDEM0397T81) Panel NICHT unterstützt.** Die Master-Aktivierung
> steuert das *gesamte* Panel von der `0x24`-Ebene an — die
> RAM-Fenster-Register (`0x44/0x45`) scopen nur, wohin Schreibvorgänge
> landen, nicht, wohin das Panel angesteuert wird, und es gibt kein
> Register, um die Ansteuerung auf ein Fenster zu begrenzen. Ein
> Fenster-Schreibvorgang lässt also veraltetes RAM außerhalb davon
> zurück, das bei der nächsten Aktivierung erneut mit Energie versorgt
> wird (zuvor fokussierte Elemente "leuchten wieder auf"). Das wurde
> auf drei Wegen bewiesen: empirisch, gegen das Datenblatt und mit
> einem eigens gebauten Isolations-Test. Nur Voll-Puffer-Schreibvorgänge
> sind kohärent, jeder Partial schreibt also beide RAM-Ebenen
> vollständig neu. `RefreshPartialRegion()` bleibt intern erhalten,
> wird aber nur je mit Ganzpanel-Grenzen aufgerufen. Der Treiber wendet
> außerdem eine Y-Gate-Leitungs-Zuordnungs-Korrektur an
> (`window_y = height-1-raw_y`) und entfernt den Pro-Partial-Hardware-
> Reset (das Datenblatt setzt nur beim Einschalten zurück).

Der Treiber kann seinen eigenen SPI-Bus initialisieren, was dieses
Board so macht: dem Panel wird `external_spi_bus=false` übergeben, und
es verwaltet `SPI3_HOST` selbst, nur-schreibend ohne MISO.

Noch nicht aus Folloup portiert:

- Aufwach-API und Display-Aufwach-Policy
- Fast-Refresh-/Basis-Pfad
- logisch-zu-roh-Display-View-Abstraktion

(Eine zurückgehaltene View-Dirty-Region-/Fenster-Partial-Refresh-Policy
wird absichtlich **nicht** verfolgt: das SSD1677-Panel kann kein
Unterfenster ansteuern, Regions-Partial-Refresh ist hier also nicht
machbar — siehe die Treiber-Anmerkung oben.)

### `components/display_service`

Das ist die App-seitige Display-Schicht. Sie komponiert `board`-Pin-
Definitionen und Strom-Helfer mit dem rohen `epaper_panel`-Treiber.

Aktueller Umfang:

- initialisiert das rohe Panel auf seinem eigenen SPI3-Bus (das Panel
  wird von den AXP2101-Rails gespeist, es gibt also kein
  GPIO-Power-Enable zu setzen)
- initialisiert den rohen SSD1677-Panel-Treiber
- rendert den Start-Splash mit `RefreshFullBase()`
- besitzt die aktuelle Portrait-Framebuffer-Surface und deren
  Refresh-Policy
- rendert den aktuellen aktiven Bildschirm (das Dashboard, Onboarding,
  eine Feature-Seite oder den Sperrbildschirm) zusammen mit dem
  passenden UI-Chrome
- betritt Panel-Sleep ohne einen speziellen Übergangs-Textbildschirm
- stellt den aktuellen Bildschirm mit einem erzwungenen vollständigen
  Refresh nach Display-Aufwachen oder Light-Sleep-Wiederherstellung
  wieder her
- loggt Panel-Refresh-Metriken und stellt Refresh-läuft-Status für
  Auto-Sleep-Blockierung bereit

Aktueller UI-Status:

- `display_service` besitzt das `ScreenId`-Bildschirm-Modell: das
  Dashboard, Onboarding, die Feature-Seiten (Vibe Check, Zusammenfassen,
  Notizen, Todos, Follow-up, Details, Einstellungen, WLAN, Zeit) und
  einen echten Sperrbildschirm
- die Statusleiste wird jetzt über `epaper_ui` gerendert
- die globale Fußzeile wird über `epaper_ui` gerendert und von
  `main/footer_runtime.cpp` gespeist
- der Sperrbildschirm nutzt seinen eigenen `epaper_ui`-Renderer und
  einen dedizierten Runtime-Helfer in `main/lock_screen_runtime.cpp`
- Overlays werden über dem aktiven Bildschirm in `DrawCurrentOverlays`
  zusammengesetzt (Z-Reihenfolge Tastatur → Toast → Auswahl-Modal →
  Karten-Modal → Sticky Note); der `RenderSnapshot` trägt den Status
  jedes Overlays (`card_modal`, `select_modal`, `keyboard`, `toast`,
  `sticky_note`)
- die Overlay-Präsentation hat zwei App-seitige Refresh-Pfade:
  - Anzeigen/Verbergen oder Fußabdruck-Änderungen bauen das Underlay
    neu, bevor das Overlay neu gezeichnet wird
  - gleiche-Sichtbarkeit-Overlay-Unruhe wie umlaufende Fokus-Updates
    oder Sticky-Note-Scrollen darf den gecachten Underlay-Snapshot
    wiederverwenden
- Sleep- und Shutdown-Indikatoren werden unmittelbar vor
  Display-Sleep-, Light-Sleep- und Deep-Sleep-Shutdown-Übergängen über
  `status_bar_runtime` gesteuert

Aktuelle entkoppelte Refresh-Regel:

- zuerst Runtime-Status im besitzenden Runtime-Helfer mutieren
- geplante, keyed UI-Präsentations-Arbeit über
  `main/ui_refresh_runtime.cpp` einreihen
- `ui_refresh_runtime` veraltete Zwischen-Updates zusammenfassen lassen
  und den neuesten Status für jede geketete Surface behalten, während
  das Panel beschäftigt ist
- den Refresh-Modus durch diese Queue als `display_service::
  RefreshRequest` (partial vs. voll) tragen; Partials sind
  änderungs-erkannt-ganzbildschirm, nicht gefenstert
- seiteneigenen Fokus-Refresh auf derselben Queue halten statt über
  eine zusätzliche App-Shell-UI-Dispatcher-Schicht zu springen
- `display_service` als alleinigen Besitzer von Framebuffer-Mutation
  und Panel-Refresh-Ausführung halten

Die aktuellen geketeten Surfaces sind:

- Overlay
- Sperrbildschirm
- Statusleiste
- Fußzeile
- eine pro Seite: Dashboard, Onboarding, Vibe Check, Zusammenfassen,
  Notizen, Todos, Follow-up, Details, Einstellungen, WLAN, Zeit

Aktuelle Refresh-Kategorien sind:

- Ganzbildschirm-Partial-Refresh: der Standard für jedes Update
  innerhalb des Bildschirms (Fokus-Umlauf, Status-Unruhe, Overlay-
  Wiederverwendung). Der Treiber diffed den gerenderten Frame gegen den
  Schatten, und das Differential-Waveform bewegt nur die geänderten
  Pixel — es gibt keine gefensterte/Regions-Variante, weil das Panel
  kein Unterfenster ansteuern kann (siehe die Treiber-Anmerkung oben)
- vollständiger Basis-Refresh: genutzt für explizite
  Vollständig-Refresh-Anfragen, Aufwach-Wiederherstellung, den
  periodischen Geist-Löschen-Takt und andere Panel-Reset-Fälle

Der aktuelle Fokus-Pfad ist:

- Seiten-Eingabe mutiert zuerst den seiteneigenen Fokus-Wahrheitswert
- die Seiten-Runtime markiert nur, ob sich die sichtbare Fußzeilen-
  Projektion tatsächlich geändert hat (sie berechnet keine
  pro-Interaktion-Dirty-Grenzen mehr — diese Maschinerie wurde
  entfernt, sobald sich Regions-Refresh als nicht machbar erwies)
- `ui_refresh_runtime` fasst die neueste `RefreshRequest` für diese
  Seite zusammen
- wenn sich die Fußzeilen-Projektion geändert hat, aktualisiert der
  eingereihte Seiten-Apply Seiten- und Fußzeilen-Status einmal
  zusammen, bevor der Refresh die Panel-Queue erreicht
- `display_service` rendert den aktiven Bildschirm neu und lässt den
  Treiber ihn gegen den Schatten diffen, wodurch nur ein
  Ganzbildschirm-Partial der geänderten Pixel angesteuert wird (oder
  komplett übersprungen wird, wenn sich nichts geändert hat)
- Overlays (Tastatur, Modale) refreshen über den Overlay-Pfad: während
  des Tippens wird nur das gecachte Underlay wiederverwendet und das
  Overlay neu gezeichnet — die darunterliegende Seite wird **nicht**
  neu gerendert, bis das Overlay schließt
- Seiten-Eintritts-Übergänge bleiben weiterhin synchron in `app_shell`
  statt über die latest-wins-Queue zu laufen, weil die App-Shell
  deterministische Reihenfolge für Touch-Provider-Einrichtung,
  Fußzeilen-Layout, Runtime-Status-Sync und Bildschirmwechsel bewahren
  muss

Die temporäre Demo-Auswahl-Maschinerie wurde entfernt; `display_service`
besitzt ein `ScreenId`-basiertes Bildschirm-Modell. Der Home-Bildschirm
rendert das echte Dashboard (`DrawHomeUnderlay` →
`epaper_ui::DrawDashboardPage`), dessen fokussierbares Menü die
Feature-Seiten öffnet.

Weil der SSD1677-Pfad sich `SPI2_HOST` mit MicroSD teilt, hängt
`display_service` davon ab, dass `storage_service` das SD-Hochfahren
bereits durchgeführt hat, wenn eine Karte steckt. Der Display-Pfad
sollte sich auf diesem Board beim Booten nicht vor den Speicher
schieben.

`display_service` besitzt App-seitige Display-Policy. Treiber-
spezifische Verdrahtung und SSD1677-Befehle müssen aus `main`
herausgehalten werden. Roher Board-Pin-Besitz bleibt in `board`, und
Low-Level-SSD1677-Befehls-Sequenzierung bleibt in `epaper_panel`.

Noch ausstehende Port-Validierungs-Anmerkungen auf Hardware:

- schnelles Auswahl-Modal-Umlaufen sollte weiterhin
  `policy=reuse_underlay_snapshot` loggen und darf keine veralteten
  Highlight-Pixel hinterlassen
- Modal- und Toast-Anzeige-/Verbergen-Übergänge sollten sauber ohne
  Geistern neu aufbauen
- das Fußzeilen-Mikrofon-Icon sollte während Aufnehmen, Speichern und
  Transkribieren aktiv bleiben
- Overlay-Verhalten sollte über Display-Sleep und Light-Sleep-
  Aufwachen hinweg korrekt bleiben
- Touch-Down-Fokus sollte vor der loslassen-basierten Aktivierung für
  Auswahl-Modal-Zeilen, Shutdown-Buttons und Fußzeilen-Elemente
  sichtbar sein
- Overlay-, Fußzeilen- und künftige Seiten-Prioritäts-Logs sollten
  während der Validierung am Gerät zur berührten Surface passen

### `components/qmi8658`

Das ist der QMI8658-6-Achsen-IMU-Treiber (3-Achsen-Beschleunigungs-
messer plus 3-Achsen-Gyroskop), am geteilten Sensor-I2C-Bus.

Aktueller Umfang:

- Hochfahren und Konfiguration
- liest Beschleunigungsmesser-Samples, genutzt von
  `device_sleep_service` für Bewegungs-Aufwachen
- hält Sleep-Policy und Schwellwerte aus dem Treiber heraus

### `components/imu_service`

Das ist die App-seitige IMU-Schicht. Sie komponiert `board`-I2C-Zugriff
mit dem generischen QMI8658-Treiber.

Aktueller Umfang:

- fügt den QMI8658 am geteilten Sensor-I2C-Bus hinzu
- konfiguriert den Beschleunigungsmesser und liest Samples
- stellt Samples für `device_sleep_runtime` bereit, das die
  Bewegungs-/Stillstands-Klassifikation und die Inaktivitäts-
  Schwellwerte besitzt

Der IMU teilt sich den Sensor-I2C-Bus mit dem PMIC und der RTC.
Auto-Sleep pollt bewusst, statt einen Interrupt anzuhängen, es gibt also
keine geteilte Interrupt-Leitung zu koordinieren.

## Hardware-Anmerkungen

- Hauptcontroller: `ESP32-S3R8`, Dual-Core Xtensa LX7 bis 240 MHz.
- Externer Flash: 16 MB. PSRAM: 8 MB.
- Geteiltes Sensor-I2C an `GPIO41` (SDA)/`GPIO42` (SCL), trägt:
  - AXP2101-PMIC an `0x34`, Interrupt an `GPIO38`
  - PCF85063-RTC an `0x51`
  - QMI8658-IMU
  - ES8311-Codec-Steuerschnittstelle
  - SHTC3-Temperatur-/Feuchtigkeit, vorhanden, aber von keiner
    Komponente angesteuert
- Keiner der beiden I2C-Pins ist ein Strapping-Pin, der Bus kann also
  während des frühen Starts erzeugt werden, ohne den Boot-Modus zu
  beeinflussen.
- Tasten sind alle active-low gegen GND: `ACTION`/BOOT auf `GPIO0`,
  Wipptaste hoch auf `GPIO4`, Wipptaste mitte/`FN` auf `GPIO5`,
  Wipptaste runter auf `GPIO6`. `GPIO0` ist der Boot-/Download-Strap,
  muss beim Reset also high lesen und wird nur je von einem Druck low
  gezogen.
- Die `PWR`-Taste ist kein GPIO. Sie ist an den AXP2101 verdrahtet und
  erscheint als Interrupts: ein Kurz-Druck-IRQ, ein Lang-Druck-IRQ,
  sobald über `IrqLevelTime` gehalten, und ein Hardware-Rail-Schnitt
  bei anhaltendem 6s-Halten.
- MicroSD nutzt den ESP32-S3-SDMMC-Controller im 4-Bit-Modus: `CLK` auf
  `GPIO16`, `CMD` auf `GPIO17`, `D0` auf `GPIO15`, `D1` auf `GPIO7`,
  `D2` auf `GPIO8`, `D3` auf `GPIO18`. Es gibt keinen Karten-Erkennungs-
  oder Power-Enable-Pin.
- Das SSD1677-E-Paper-Panel besitzt einen eigenen `SPI3_HOST`,
  nur-schreibend ohne MISO: `BUSY` auf `GPIO3`, `DC` auf `GPIO9`, `CS`
  auf `GPIO10`, `SCK` auf `GPIO11`, `MOSI` auf `GPIO12`, `RST` auf
  `GPIO46`. Das Panel wird von den AXP2101-Rails gespeist, es gibt
  also kein GPIO-Power-Enable.
- Weil sich das Panel keinen Bus mit MicroSD teilt, gilt hier nichts
  von der geteilten-SPI-Serialisierung des Sticky: es gibt keinen
  Bus-Guard und keine Reihenfolge-Anforderung zwischen SD mounten und
  Display hochfahren.
- Das E-Paper-Panel hat 800 x 480 rohe Querformat-Pixel.
  `display_service` zeichnet Hochformat-Inhalt, indem es logische
  480 x 800-Koordinaten auf den rohen Framebuffer abbildet. Zu
  beachten: Hochformat-`x` bildet auf die Gate-Leitung des Panels ab
  (`raw_y = height - 1 - x`), weshalb eine große Füllung mit einem
  gate-periodischen Dither-Muster bei einem Partial-Refresh sichtbare
  Streifenbildung erzeugt.
- ES8311-Audio streamt über I2S0 bei 16 kHz vollduplex: `MCLK` auf
  `GPIO13`, `BCLK` auf `GPIO14`, `WS` auf `GPIO47`, `DIN` auf `GPIO21`,
  `DOUT` auf `GPIO48`. Die NS4150B-Verstärker-Freigabe liegt auf
  `GPIO39`.
- Es gibt keinen Power-Latch, kein Ladegerät-Freigabe-GPIO und keine
  ADC-Strom-Eingangs-Erfassung. Der AXP2101 besitzt Rails, Laden und
  Akku-Telemetrie über I2C.

## Konfiguration

Konfiguration ist dateibasiert und sollte reproduzierbar bleiben:

- `sdkconfig.defaults` hält die beabsichtigten Projekt-Standardwerte.
- `sdkconfig` hält die aufgelöste ESP-IDF-Konfiguration.
- `partitions.csv` definiert die OTA-Partitionstabelle.

Projekt-spezifische Kconfig-Optionen liegen unter `Folloup Settings`.
Auto-Sleep stellt aktuell reproduzierbare Build-Zeit-Standardwerte für
Display-Sleep- und Light-Sleep-Timeout-Sekunden bereit; `0` deaktiviert
die entsprechende Stufe, und ein von null verschiedener Light-Sleep-
Timeout muss größer oder gleich dem Display-Sleep-Timeout sein.

Die Partitionstabelle enthält aktuell:

- `nvs`
- `otadata`
- `phy_init`
- `ota_0`
- `ota_1`

Rollback ist aktiviert mit
`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`. Weil Rollback aktiviert ist,
muss die Anwendung den OTA-Validierungs-Hook in `app_main()` oder
gleichwertigem frühen Startcode behalten.

## Abhängigkeits-Richtung

Diese Abhängigkeits-Richtung nutzen:

```text
app / integration code
  -> power_service
       -> board -> ESP-IDF drivers
       -> axp2101 -> ESP-IDF I2C/GPIO drivers
       -> pcf85063 -> ESP-IDF I2C driver
  -> power_key_runtime (main/)
       -> board (GetPmic)
       -> axp2101
       -> device_sleep_service / device_sleep_runtime
  -> button_service -> espressif/button
  -> feedback_service
       -> system_sound_service -> audio_hal -> ESP-IDF I2S driver
  -> device_sleep_service
  -> storage_service
       -> sd_card -> ESP-IDF SDMMC/FATFS drivers
  -> recording_service
       -> board (GetAudioCodec)
       -> audio_hal -> ESP-IDF I2S/I2C drivers
  -> playback_service
       -> board (GetAudioCodec)
       -> audio_hal
       -> recording_service (RecordedClip only)
  -> recording_session_service
       -> recording_service
       -> playback_service
       -> system_sound_service
  -> display_service
       -> epaper_panel -> ESP-IDF SPI/GPIO drivers
  -> imu_service
       -> qmi8658 -> ESP-IDF I2C driver
```

Vermeiden, `axp2101`, `pcf85063`, `sd_card`, `epaper_panel`, `qmi8658`
oder `audio_hal` von `board` abhängig zu machen; das würde generische
Treiber board-spezifisch machen. `board` komponiert sie mit der
Waveshare-Pin-Zuordnung, und App-seitige Dienste erreichen die
Hardware über `board`-Zugriffsmethoden wie `GetPmic()` und
`GetAudioCodec()`.

`playback_service` hängt von `recording_service` nur wegen des
`RecordedClip`-Typs ab, den es streamt. Die umgekehrte Kante darf nicht
existieren: `recording_service` besitzt Aufnahme und Clip-Besitz und
bekommt keinen Ausgabe-Pfad.
