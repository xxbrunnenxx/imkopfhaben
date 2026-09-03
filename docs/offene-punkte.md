# Offene Punkte — folloup-waveshare / lokale KI

Stand: 2026-09-03. Noch keine GitHub-Issues, nur damit hier nichts
verloren geht. Reihenfolge = ungefähre Priorität.

## Blocker

- **Kein echtes Board zum Flashen/Testen.** Bestellt, noch nicht
  angekommen. Der einzige verbleibende Blocker für einen echten
  End-to-End-Probelauf — beide Server-Pfade (Transkription,
  Chat-Completion) sind bereits live gegen die echten Firmware-Request-
  Formate bestätigt.

## Vor dem Probelauf klären

- **Kein systemd-Unit für LM Studio.** Läuft nur manuell auf
  `0.0.0.0:1234`, übersteht keinen Kraken-Neustart (Bindung geht dann auf
  `127.0.0.1`-only zurück, muss von Hand mit `lms server start --bind
  0.0.0.0` neu gesetzt werden). Für den Prototyp-Testpfad selbst nicht
  zwingend (die Transkription braucht LM Studio nicht, nur die
  Chat-Completion/Zusammenfassung), aber der fragilste Punkt im aktuellen
  Stand. Entwurf liegt in `kraken-arche` PR #1 (bereits gemerged, aber
  unverifiziert — kein Reboot-Test).
- **Transkriptions-Readiness-Health-Check fehlt.** Prüft aktuell nur "ist
  eine URL konfiguriert", nicht ob der Server wirklich erreichbar ist —
  ein offline Kraken blockiert bis zu 30s pro Versuch statt sofort zu
  scheitern. Design-Entscheidung, kein Bug-Fix. Für den Probelauf laut
  Besitzer akzeptiert.

## Bekannt, bewusst nicht gefixt (aus dem Review, Besitzer-Entscheidung)

- **Unnötige Transkript-Text-Ladevorgänge im Retry-Scan.**
  `TryRetryOldestUnsentRecording()` liest bei jedem Durchlauf über
  `ListRecordings()` den vollen Transkript-Text jeder schon
  transkribierten Aufnahme mit, obwohl nur `has_transcript` gebraucht
  wird. Läuft alle 10 Minuten + nach jeder erfolgreichen Speicherung.
  Für den aktuellen Prototyp-Umfang (wenige Notizen) unkritisch, würde
  erst über Wochen/Monate mit großem Archiv relevant.
- **Doppelter Busy/Notify/Try-Catch-Wrapper im Portal-Frontend**
  (`webserver/src/portal/providerKeys.ts`). `saveLocalAiBaseUrl`/
  `resetLocalAiBaseUrl` bauen dieselbe Hülle nach, die
  `saveProviderKey`/`clearProviderKey` schon haben. Echte Duplikation,
  aber die beiden Pfade haben unterschiedliche Settings-Anwendungslogik
  (maskiertes Secret vs. offene URL) — nur die äußere Hülle wäre sicher
  extrahierbar, kein 1:1-Fix ohne Umbau von `applyProviderSettings`.

## Offene Entscheidung

- **`.agents/skills/esp32-firmware-engineer/` weiterhin unübersetzt
  (bewusst).** `SKILL.md` + 17 Referenz-Dateien, bestätigt vendoriertes/
  mitgeliefertes Claude-Code-Skill-Paket (kam im allerersten Commit
  "init: setup a new esp-idf project" rein), kein Projekt-Dokument im
  eigentlichen Sinn — deshalb bei der Doku-Übersetzung nicht ungefragt
  mitgemacht. Alle echten Projekt-Dokumente (`docs/*.md`, `README.md`,
  `AGENTS.md`, `webserver/README.md`, `scripts/README.md` — 10 Dateien
  insgesamt) sind seit PR #6/#7 vollständig deutsch, repo-weit
  gegengeprüft (`find . -iname "*.md" -not -path "./build/*" -not -path
  "./managed_components/*" -not -path "*/node_modules/*"`). **Noch
  offen:** Freigabe, ob der Skill-Ordner trotzdem übersetzt werden soll.

## Später, nicht dringend

- **Offline-Warteschlange fürs Board ist gebaut, aber nie live getestet**
  (kein Board). Code-seitig verifiziert, Review-Funde daran (Deadlock,
  Race) sind in PR #3 gefixt.
- Kein `transcribe_url`-Feld in der Portal-UI (nur `base_url` editierbar).
