# Offene Punkte — folloup-waveshare / lokale KI

Stand: 2026-09-02. Noch keine GitHub-Issues, nur damit hier nichts
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

## Nicht erledigt, obwohl beauftragt

- **Doku-Übersetzung ins Deutsche unvollständig.** Auftrag war "alle
  Dokumente auf `folloup-waveshare` prüfen, falls nicht deutsch,
  übersetzen". Tatsächlich übersetzt: `docs/*.md` + Root-`README.md` (7
  Dateien). **Übersehen:** `AGENTS.md` (Root), `webserver/README.md`,
  `scripts/README.md` — alle drei nach wie vor englisch. Zusätzlich noch
  nicht geklärt: `.agents/skills/esp32-firmware-engineer/` (`SKILL.md` +
  17 Referenz-Dateien) — sieht nach vendorierter/mitgelieferter Skill-
  Definition aus, kein Projekt-Dokument im eigentlichen Sinn, daher
  bewusst nicht ungefragt mitübersetzt.
  **Warum das passiert ist:** beim Scope-Check zu Beginn wurde nur
  `ls docs/*.md README.md` ausgeführt — eine Prüfung, die auf den beiden
  Orten aufbaute, mit denen in der Sitzung bis dahin ohnehin gearbeitet
  wurde, nicht auf einer echten repo-weiten Suche. Es wurde angenommen,
  dass alle Dokumentation dort liegt, statt das zu verifizieren. Erst
  durch eine Besitzer-Rückmeldung ("die sind mir DIREKT ins Auge
  gefallen") aufgefallen, nicht durch eigene Prüfung. Vollständige Liste
  aller `.md`-Dateien im Repo: `find . -iname "*.md" -not -path
  "./build/*" -not -path "./managed_components/*"`.
  **Noch offen:** Freigabe, ob die drei übersehenen Dateien (und ggf. die
  Skill-Referenzen) nachgeholt werden sollen.

## Später, nicht dringend

- **Offline-Warteschlange fürs Board ist gebaut, aber nie live getestet**
  (kein Board). Code-seitig verifiziert, Review-Funde daran (Deadlock,
  Race) sind in PR #3 gefixt.
- Kein `transcribe_url`-Feld in der Portal-UI (nur `base_url` editierbar).
