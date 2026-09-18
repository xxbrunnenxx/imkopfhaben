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
