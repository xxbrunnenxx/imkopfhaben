# Followup-Einrichtungsportal

Web-UI zur Einrichtung eines Followup-Geräts über dessen WLAN-Access-Point.
Das Portal ist mit TypeScript + Vite gebaut und **in die Firmware
eingebettet** (siehe [Build & Einspielen](#build--einspielen-in-die-firmware))
— es wird vom HTTP-Server der `wifi_service`-Komponente ausgeliefert,
solange sich das Gerät im AP-Modus befindet.

Es ist vom byte90-Captive-Portal abgeleitet, heruntergebrochen auf die
drei Dinge, die Followup einrichtet:

- **WLAN** — scannen, verbinden, trennen, Live-Status
- **Lokaler KI-Server** — Basis-URL setzen/zurücksetzen (kein Schlüssel,
  es ist eine LAN-Adresse)
- **Zeit & Zeitzone** — Zeitzone wählen oder Datum/Uhrzeit manuell setzen

Das Frontend gliedert sich in:

- eigenständige Web Components in `src/components/`
- Feature-Controller in `src/portal/`
- eine schlanke `src/main.ts`-Bootstrap-/Orchestrierungs-Schicht

## Vom Portal genutzte API-Endpunkte

Diese werden von der Followup-Firmware (`wifi_service`, `timezone_service`,
`local_ai_service`) ausgeliefert, solange sich das Gerät im
Access-Point-Modus befindet.

WLAN (`wifi_service`)

- `GET /api/scan`
- `GET /api/status`
- `POST /api/configure`
- `POST /api/disconnect`

Zeit / Zeitzone (`timezone_service`)

- `GET /api/timezone/list`
- `GET /api/settings/time`
- `PATCH /api/settings/time`
- `GET /api/runtime/time`

Lokale KI (`local_ai_service`)

- `GET /api/settings/local_ai`
- `PATCH /api/settings/local_ai`
- `POST /api/settings/local_ai/reset`

## Lokale Entwicklung

Vom Repo-Root aus:

```bash
cd webserver
npm install
npm run dev
```

Vite liefert die App unter `http://localhost:5173/` aus. Das Portal macht
absolute `/api/...`-Aufrufe, ein reiner Dev-Server hat also kein Backend —
das eigene Gerät auf den AP eines Followup-Geräts richten (oder `/api`
dorthin proxen), um die Abläufe gegen echte Firmware zu prüfen.

## Build & Einspielen in die Firmware

Die Firmware bettet eine kleine, feste Menge an Dateien (`index.html`,
`index.js`, `index.css`) über `EMBED_FILES` in
`components/wifi_service/CMakeLists.txt` ein. Um das Portal nach jeder
Frontend-Änderung zu aktualisieren:

```bash
cd webserver
npm run build
cp dist/index.html dist/index.js dist/index.css ../components/wifi_service/portal/
```

Danach die Firmware neu bauen. `wifi_service` liefert sie unter `/`,
`/index.js` und `/index.css` aus.

`webserver/` ist die Quelle der Wahrheit. Die Kopien unter
`components/wifi_service/portal/` sind der versionierte, eingebettete
Build-Output — mit dem obigen Befehl aktualisieren, nicht von Hand
bearbeiten.

## Projektstruktur

```text
webserver/
  index.html              # Portal-Markup (WLAN-/Lokale-KI-/Zeit-Karten)
  src/assets/             # Inline-SVGs (?raw), inkl. followup_logo.svg
  src/components/         # eigenständige Web Components
  src/portal/             # Feature-Controller, API-Helfer, DOM-Verdrahtung, Typen
  src/gradualBlur.ts
  src/main.ts             # Bootstrap und Orchestrierung
  src/portal.css          # globale Seiten-/Layout-Stile
  dist/                   # Build-Output (generiert, gitignored)
components/wifi_service/portal/   # eingebetteter Build-Output, von der Firmware ausgeliefert
```

## Portal-Quellstruktur

- `src/components/`
  Gemeinsam genutzte Web Components wie `Button`, `Input`, `Select`,
  `Card`, `Toast`, `NetworkStatus`, `NetworkList` und `BottomSheet`.

- `src/portal/`
  Portal-spezifische Anwendungslogik, aufgeteilt nach Zuständigkeit:
  - `api.ts` — typisierte API-Fetch-Helfer
  - `wifi.ts` — WLAN-Scan-/Verbinden-/Trennen-Ablauf
  - `time.ts` — Zeitzone + manuelles Datum/Uhrzeit
  - `providerKeys.ts` — Speichern/Zurücksetzen der lokalen-KI-Server-URL,
    Speichern/Löschen des OpenAI-API-Schlüssels
  - `dom.ts` — Element-Lookups
  - `events.ts` — DOM-Event-Bindungen
  - `uiState.ts` — Aktiviert-/Deaktiviert-Zustand von Buttons/Eingaben
  - `duration.ts`, `uiHelpers.ts` — gemeinsame Helfer
  - `constants.ts`, `types.ts` — gemeinsame Konstanten und TypeScript-Typen

## Skripte

- `npm run dev` — Vite-Dev-Server starten
- `npm run build` — Typprüfung (`tsc`) und Produktions-Assets nach `dist/` bauen
- `npm run preview` — Produktions-Build in der Vorschau ansehen
- `npm run lint` — ESLint ausführen
