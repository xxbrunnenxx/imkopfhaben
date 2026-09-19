# Offene Punkte — folloup-waveshare / lokale KI

Stand: 2026-09-13 (Board da, erster echter Probelauf gelaufen — der
frühere Blocker "kein Board" ist damit weg und deshalb hier entfernt).
Noch keine GitHub-Issues, nur damit hier nichts verloren geht.
Reihenfolge = ungefähre Priorität.

## Vor dem nächsten Probelauf klären

- **Portal-Check nach OpenAI-Code-Entfernung noch offen.** Der
  ungenutzte OpenAI-Provider-Code wurde aus dem Portal-Frontend
  entfernt (Commit `3781f3b`), Build/Flash bestätigt grün — aber der
  Besitzer hat das Portal (WLAN-Einrichtung) danach noch nicht selbst
  live geöffnet und geprüft, ob alles wie gewohnt aussieht/funktioniert
  (insbesondere: keine OpenAI-Karte mehr, lokale-KI-Feld weiterhin
  funktionsfähig). Prüfung für die nächsten Tage vom Besitzer
  angekündigt, noch nicht erledigt.
- **Kein systemd-Unit für LM Studio — bewusst so, per Start-Skript
  gelöst (Stand 2026-09-18).** Kraken wurde zwischenzeitlich neu
  aufgesetzt; LM Studio samt Modell war komplett weg und musste von Hand
  neu installiert, Modell neu geladen und der Server manuell gestartet
  werden. Entscheidung des Besitzers: **kein Autostart/systemd** — der
  Pi 5 soll im Ruhezustand lastfrei bleiben. Statt eines Dauerdienstes
  gibt es jetzt im Notizbuch-Repo (`imkopfhaben-public`)
  `brain/imkopfhaben-start.sh` (fährt LM Studio :1234 + brain :8000 hoch,
  wartet bis beide antworten) und `brain/imkopfhaben-stop.sh` (beendet
  beide, macht den Pi lastfrei). Nach jedem Reboot einmal von Hand
  starten, Anleitung in `imkopfhaben-public/brain/ANLEITUNG.md`. Der alte
  `lmstudio-server.service`-Entwurf in `kraken-arche` PR #1 wird damit
  nicht mehr gebraucht. Für den reinen Transkriptions-Testpfad ist LM
  Studio ohnehin nicht nötig (nur die Chat-Completion/Zusammenfassung).
- **Transkriptions-Readiness-Health-Check fehlt.** Prüft aktuell nur "ist
  eine URL konfiguriert", nicht ob der Server wirklich erreichbar ist —
  ein offline Kraken blockiert bis zu 30s pro Versuch statt sofort zu
  scheitern. Design-Entscheidung, kein Bug-Fix. Für den Probelauf laut
  Besitzer akzeptiert.

## Bekannt, bewusst nicht gefixt (aus dem Review, Besitzer-Entscheidung)

- **Font-Tabelle deckt Latin-1 0x20–0xFC ab** (`scripts/
  generate_epaper_fonts.py`, `kCharFirst`/`kCharLast`), 2,44× größer als
  reines ASCII. Grund: `FindGlyph()` (`components/epaper_ui/
  bitmap_font.cpp`) indiziert direkt per `codepoint - first_char` in ein
  zusammenhängendes Array — da die deutschen Umlaute (0xE4–0xFC) weit
  von ASCII (0x20–0x7E) entfernt liegen, ist die aktuelle Spanne bereits
  die kleinstmögliche zusammenhängende. Eine echte Verkleinerung
  bräuchte eine nicht-zusammenhängende Lookup-Tabelle in Generator UND
  Renderer — größerer Eingriff, kein minimalinvasiver Fix.

## Später, nicht dringend

- **Offline-Warteschlange fürs Board weiterhin nicht live getestet** —
  jetzt zwar ein Board da und der Online-Pfad (Aufnahme → sofortige
  Transkription) mehrfach bestätigt, aber der eigentliche Offline-Fall
  (Aufnahme ohne Netz, späterer automatischer Retry beim Reconnect)
  wurde noch nicht gezielt durchgespielt. Code-seitig verifiziert,
  Review-Funde daran (Deadlock, Race) sind in PR #3 gefixt.
- Kein `transcribe_url`-Feld in der Portal-UI (nur `base_url` editierbar).

## Refresh-Politik bei Screenwechseln (2026-09-19)

- **Gebaut, geflasht, Logik geprüft — Augenschein am Gerät noch offen.**
  Screenwechsel laufen jetzt auf der schnellen OTP-Wellenform (`kFast`)
  statt der mode-1-Vollwellenform; jeder achte Wechsel bleibt `kFull` als
  Ghosting-Flush. Eingriff zentral in `SetCurrentScreen`
  (`components/display_service/display_service.cpp`), nicht an den 20
  Aufrufstellen.
- Geprüft: Build grün, Flash auf `/dev/ttyACM0` verifiziert (Hash ok),
  Bootlog zeigt den ersten Wechsel erwartungsgemäß als `mode=full`, die
  Politik selbst per Host-Testprogramm (8er-Zyklus, `kPartial`/`kFast`
  bleiben unverändert).
- **Ungeprüft:** wie die Folgewechsel am echten Panel aussehen. Screens
  wechseln nur per physischem Tastendruck, ferngesteuert nicht auslösbar
  — im 5-Minuten-Leerlauf-Mitschnitt kam kein einziger Screenwechsel vor.
  Der Besitzer muss einmal durch die Menüs gehen und sagen, ob das
  Blitzen weg ist und ob nach ~8 Wechseln genug Ghosting weggeht. Falls
  zu viel Ghosting bleibt: `kGhostFlushEveryNScreenChanges` verkleinern.

### Nachtrag 2026-09-19: der gemeldete Blitz kam woanders her

Der Besitzer hat das Gerät benutzt, während ein 5-Minuten-Mitschnitt
lief, und meldete weiterhin Blitzen. Der Mitschnitt zeigt warum — es war
**nicht** der Screenwechsel-Pfad:

- Im ganzen Nutzungsfenster gab es **keinen einzigen Screenwechsel**.
  Alle Display-Kommandos: `mode=partial scope=region
  source=dashboard_page` (Scrollen mit der DOWN-Taste im Dashboard).
- Genau **ein** Voll-Refresh, bei Zeitstempel 119264:
  `busy=2108938us` — die 2,1 s. Direkt davor sechs Partials à ~510 ms.
- Ursache: `RefreshPartialFullScreen` in
  `components/epaper_panel/ssd1677_driver.cpp`. Nach
  `kMaxPartialRefreshesBeforeFlush = 8` Partials erzwingt
  `CanPartialRefresh` einen `RefreshFullBase()` gegen das Verblassen.
  Beim Durchscrollen einer Liste ist diese Schwelle in Sekunden erreicht.

Die heutige `SetCurrentScreen`-Änderung bleibt richtig und wirksam (sie
nimmt das Blitzen beim Seitenwechsel), trifft dieses Blitzen aber nicht.

**Offen — Entscheidung des Besitzers**, weil es ein echter Kompromiss ist
(Blitzen gegen Kontrast):

1. Schwelle hochsetzen (z. B. 8 → 20): seltener Blitzen, dafür wird das
   Bild zwischendurch sichtbar blasser.
2. Flush auf die schnelle Wellenform legen: aus 2,1 s werden ~0,8 s,
   klärt aber weniger gründlich. Der Kommentar im Treiber rät davon ab,
   weil `kFast` auf diesem Panel bei geringerem Kontrast landet und ein
   verblasstes Bild damit nicht wieder aufgefrischt wird.
3. Flush verschieben statt auslösen: beim Scrollen weiterzählen, aber den
   Voll-Refresh erst fahren, wenn der Nutzer kurz nichts tut. Dann blitzt
   es nie mitten in der Bewegung. Aufwendiger, aber der einzige Weg, der
   beides behält.

### Gelöst 2026-09-19: Flush wartet auf den Leerlauf

Umgesetzt wurde Variante 3. Zwei Schwellen statt einer:

- `kMaxPartialRefreshesBeforeFlush = 8` markiert den Flush nur noch als
  **fällig** (`EpaperPanel::DeferredFlushPending()`), erzwingt ihn nicht.
- `kMaxPartialRefreshesHardCap = 60` ist die harte Grenze, ab der auch
  mitten in der Bewegung geflusht wird, damit ein Dauerstrom von
  Partials das Bild nicht beliebig verblassen lässt.
- `DisplayTask` wartet nicht mehr `portMAX_DELAY` auf die Queue, sondern
  2,5 s. Läuft der Wartezeitraum leer ab und ist ein Flush fällig, wird
  er dort gefahren — im Leerlauf, nicht während des Scrollens.

**Gemessen am Gerät (Mitschnitt /tmp/v2.txt, Besitzer hat live bedient):**

| | vorher | nachher |
|---|---|---|
| Screenwechsel | 2,11 s (`full`) | 1,71 s (`fast`) |
| Scrollschritt | 0,51 s | 0,51 s |
| Blitz mitten im Scrollen | ja | keiner |

Im Mitschnitt liefen acht Partials am Stück (t=24516..34256) — genau die
Konstellation, die vorher den 2,1-s-Blitz auslöste. Diesmal kam keiner.
Sechs echte Screenwechsel (Lockscreen an/aus, VibeCheck, Summarize,
2× Home) fuhren alle als `mode=fast`. Besitzer-Urteil: "sieht gut aus,
ich hab noch keinen unerwünschten refresh gehabt."

**Anmerkung für später:** `kFast` spart auf diesem Panel nur ~0,4 s
(1,71 s statt 2,11 s). Der spürbare Gewinn kommt weniger aus der Dauer
als daraus, dass der unerwartete Flush mitten in der Bewegung weg ist.
Falls das Bild bei langem Scrollen doch zu blass wird, ist
`kMaxPartialRefreshesHardCap` die Stellschraube (60 herunter).
