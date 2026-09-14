# Offene Punkte — folloup-waveshare / lokale KI

Stand: 2026-09-13 (Board da, erster echter Probelauf gelaufen — der
frühere Blocker "kein Board" ist damit weg und deshalb hier entfernt).
Noch keine GitHub-Issues, nur damit hier nichts verloren geht.
Reihenfolge = ungefähre Priorität.

## Vor dem nächsten Probelauf klären

- **Kein systemd-Unit für LM Studio — weiterhin offen, heute erneut
  bestätigt.** Kraken wurde zwischenzeitlich neu aufgesetzt; LM Studio
  samt Modell war komplett weg und musste von Hand neu installiert,
  Modell neu geladen und der Server erneut manuell mit `lms server
  start --bind 0.0.0.0` gestartet werden. Genau das fragile Verhalten,
  das dieser Punkt schon vorher beschrieb, jetzt real eingetreten.
  Für den Prototyp-Testpfad selbst nicht zwingend (die Transkription
  braucht LM Studio nicht, nur die Chat-Completion/Zusammenfassung).
  Entwurf liegt in `kraken-arche` PR #1 (gemerged, weiterhin
  unverifiziert — kein Reboot-Test).
- **Transkriptions-Readiness-Health-Check fehlt.** Prüft aktuell nur "ist
  eine URL konfiguriert", nicht ob der Server wirklich erreichbar ist —
  ein offline Kraken blockiert bis zu 30s pro Versuch statt sofort zu
  scheitern. Design-Entscheidung, kein Bug-Fix. Für den Probelauf laut
  Besitzer akzeptiert.

## Bekannt, bewusst nicht gefixt (aus dem Review, Besitzer-Entscheidung)

- **Doppelter Busy/Notify/Try-Catch-Wrapper im Portal-Frontend**
  (`webserver/src/portal/providerKeys.ts`). `saveLocalAiBaseUrl`/
  `resetLocalAiBaseUrl` bauen dieselbe Hülle nach, die
  `saveProviderKey`/`clearProviderKey` schon haben. Echte Duplikation,
  aber die beiden Pfade haben unterschiedliche Settings-Anwendungslogik
  (maskiertes Secret vs. offene URL) — nur die äußere Hülle wäre sicher
  extrahierbar, kein 1:1-Fix ohne Umbau von `applyProviderSettings`.
- **Footer-Icon-Entfernungslogik über ~30 Stellen/3 Layer dupliziert**
  (`components/page_navigation/navigation_model.cpp:74` und die
  jeweiligen Pro-Seiten-Runtimes). Jede Seite baut ihre
  Footer-Fokus-Projektion einzeln nach statt über einen gemeinsamen
  Helfer — echte Duplikation, aber eine saubere Konsolidierung wäre ein
  größerer Umbau über alle Seiten hinweg, keine minimalinvasive
  Änderung.
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
