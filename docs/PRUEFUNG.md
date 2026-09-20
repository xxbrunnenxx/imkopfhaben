# imkopfhaben-esp32 — Prüfprotokoll

> Angelegt 19.09.2026

Jede Behauptung braucht den **Handgriff, der sie umwerfen würde**. Ist er
nicht gelaufen, gilt die Behauptung als ungeprüft — nicht als wahr.

Die Datei existierte bisher leer. Angelegt wurde sie, als die
Refresh-Änderungen belegt werden mussten.

**Ergebnis-Werte:** `belegt` · `offen` · `widerlegt`

**Keine Notizinhalte in diesem Repo.** Das Repo liegt auf GitHub, die
Aufnahmen des Besitzers gehoeren nicht dorthin. Belege beschreiben
deshalb, *dass* ein Text korrekt ankam, in welcher Laenge und mit
welchem Zeitverhalten — nie *was* darin stand. Wer einen Wortlaut zum
Nachvollziehen braucht, liest ihn am Geraet oder in der lokalen
Mitschrift auf Kraken (`~/imkopfhaben-mitschrift/`, nicht versioniert).

Handgriff für alle Log-Belege (Board an `/dev/ttyACM0`):

```sh
python3 -c "
import serial,time
s=serial.Serial('/dev/ttyACM0',115200,timeout=1); end=time.time()+90
print('\n'.join(l for l in iter(lambda: s.readline().decode('utf8','replace').rstrip(), None) if time.time()<end))"
```

---

## Refresh-Verhalten des Panels

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Firmware baut | `idf.py build` | belegt — `0x375dc0` Bytes, 56 % frei | 19.09. |
| Firmware läuft auf dem Board | `idf.py -p /dev/ttyACM0 flash` | belegt — „Hash of data verified" | 19.09. |
| Screenwechsel fahren `kFast`, nicht `kFull` | Log auf `panel drive: mode=` prüfen | belegt — 6 Wechsel, alle `mode=fast` (Lockscreen an/aus, VibeCheck, Summarize, 2× Home) | 19.09. |
| `kFast` ist messbar kürzer als `kFull` | `refresh metrics: busy=` vergleichen | belegt — 1,71 s gegen 2,11 s. **Nur ~0,4 s Gewinn**, nicht Tag und Nacht | 19.09. |
| Jeder 8. Screenwechsel bleibt `kFull` | Host-Testprogramm über 24 Wechsel | belegt — 3× full, 21× fast, `kPartial`/`kFast` unverändert durchgereicht | 19.09. |
| Erster Screenwechsel nach Boot ist `kFull` | Bootlog, erster Eintrag | belegt — `source=show_home_screen mode=full`, Index 0 der Politik | 19.09. |
| Acht Partials am Stück lösen **keinen** Blitz mehr aus | Liste durchscrollen, Log auf `busy>1,5s` prüfen | belegt — t=24516..34256, acht Partials à 0,51 s, kein Voll-Refresh | 19.09. |
| Der gemeldete Blitz kam aus dem Partial-Pfad, nicht vom Screenwechsel | Mitschnitt während echter Nutzung auswerten | belegt — im Fenster kein einziger Screenwechsel, alle Kommandos `scope=region source=dashboard_page`, genau ein `busy=2108938us` bei t=119264 | 19.09. |
| Harte Grenze bei 60 Partials greift | `scripts/flush-politik-test.sh` | belegt — erster erzwungener Voll-Refresh exakt bei Partial 61 | 19.09. |
| Leerlauf holt den fälligen Flush nach und setzt den Zähler zurück | `scripts/flush-politik-test.sh` | belegt | 19.09. |
| Leerlauf ohne fälligen Flush blitzt nicht | `scripts/flush-politik-test.sh` | belegt — kein Blitzen im Ruhezustand | 19.09. |
| Scrollen mit Pausen blitzt nie mitten in der Bewegung | `scripts/flush-politik-test.sh` | belegt — 20 Runden à 8 Partials, kein Blitz innerhalb einer Runde, 20 Flüsche im Leerlauf | 19.09. |
| Screenwechsel nach 8 Partials lässt den Flush entfallen | `scripts/flush-politik-test.sh` | belegt — kein nachträglicher Blitz. Erklärt, warum im Nutzungs-Mitschnitt nie ein Flush auftauchte | 19.09. |
| Im Displayschlaf wird kein Flush auf das schlafende Panel gefahren | `scripts/flush-politik-test.sh` | belegt — auch kein Altlast-Flush nach dem Aufwachen | 19.09. |
| Ein ruhiges Gerät wird nicht alle 2,5 s geweckt | `scripts/flush-politik-test.sh` | belegt — ohne fälligen Flush blockiert `DisplayTask` unbegrenzt wie zuvor | 19.09. |
| **Aufgeschobener Flush feuert auf der Hardware** | Schwelle temporär auf 3, flashen, Log lesen | **belegt** — `Deferred ghosting flush: idle after 4 partials` bei t=18136, letzter Partial bei t=15636: exakt 2,5 s Ruhe davor. Schwelle danach auf 8 zurückgesetzt, neu geflasht, gegengeprüft (kein Flush mehr) | 19.09. |
| Geflashte Firmware entspricht dem gebauten Stand | `esptool verify_flash 0x20000 build/folloup_sticky.bin` | belegt — „verify OK (digest matched)" | 19.09. |
| Test läuft aus einem frischen Klon | `git clone` in leeres Verzeichnis, dann `scripts/flush-politik-test.sh` | belegt — alle 23 Fälle grün, keine Abhängigkeit von meiner Arbeitskopie | 19.09. |
| Gerät blitzt im laufenden Betrieb nicht mehr | 3 min Mitschnitt, `busy>1,5s` suchen | belegt — kein einziger Voll-Refresh, kein Absturz | 19.09. |
| Stale `flush_due` im Displayschlaf heilt sich selbst | `scripts/flush-politik-test.sh` | belegt — kostet höchstens einen zusätzlichen Timeout, danach blockiert die Aufgabe wieder unbegrenzt | 19.09. |
| Keine Reste der Prüf-Schwelle im Baum | `grep -rn TEMP-PRUEFUNG` ohne `build`/`.git` | belegt — kein Treffer; Schwellen stehen auf 8 und 60 | 19.09. |

## KI-Stern in der Statusleiste (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Der Stern haengt an `local_ai_service.runtime.ready` | `status_bar_runtime.cpp:64`, `show_ai_icon = wifi.connected && ready` | belegt — Code gelesen, einzige Setzstelle | 19.09. |
| Der Stern fehlte, weil im NVS eine veraltete Server-Adresse stand | Bootlog lesen | belegt — `base_url=http://192.168.0.146:1234/v1/ source=nvs`, Kraken ist `192.168.178.215`; Folge: `transport_error ESP_ERR_HTTP_CONNECT` | 19.09. |
| Die veraltete Adresse war im WLAN-Betrieb gar nicht korrigierbar | `curl http://192.168.178.75/api/settings/local_ai` vor dem Fix | belegt — Exit 7, Verbindung verweigert; `StartConfigPortal()` lief nur im AP-Modus, und es gibt kein Feld dafuer auf der Einstellungsseite des Geraets | 19.09. |
| Die Weboberflaeche laeuft jetzt auch im WLAN-Betrieb | `curl http://192.168.178.75/api/settings/local_ai` nach dem Fix | belegt — HTTP 200 mit vollem Einstellungs-JSON | 19.09. |
| Zuruecksetzen auf den eingebauten Standard heilt den Stern | `curl -X POST .../api/settings/local_ai/reset`, dann Runtime lesen | belegt — `base_url_source=built_in`, `http://kraken.local:1234/v1/`, danach `ready=true`, `Connected to google/gemma-4-e2b`, HTTP 200 | 19.09. |
| Ohne Server meldet das Geraet ehrlich `ready=false` | `imkopfhaben-stop.sh`, dann Readiness erneut ausloesen | belegt — `ready=false`, `Local AI server unreachable` | 19.09. |
| **Der Stern kommt von selbst zurueck, ohne Neustart und ohne WLAN-Ereignis** | Server aus -> `ready=false`, Server per `imkopfhaben-start.sh` hoch, 70 s warten, Runtime lesen | **belegt** — `ready=true`, `Connected to google/gemma-4-e2b`. Das ist der Beweis fuer den neuen 60-s-Readiness-Timer; vorher lief die Pruefung nur einmal pro WLAN-Ereignis | 19.09. |
| Firmware mit beiden Fixes baut und laeuft | `idf.py build`, `idf.py -p /dev/ttyACM0 flash` | belegt — `0x375f40` Bytes, 56 % frei; „Hash of data verified", Board bootet und verbindet sich | 19.09. |
| Captive-DNS bleibt dem AP-Modus vorbehalten | `StartConfigPortal(captive_dns)`, Aufrufstellen gelesen | belegt — nur `EnterAccessPointModeNow` und der AP-Zweig uebergeben `true` | 19.09. |

## Transkription bricht ab (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Der Brain-Server ist nicht schuld | `brain.log` zur Fehlschlagszeit lesen | belegt — alle Anfragen HTTP 200, Whisper lieferte einen korrekten, vollstaendigen Text zurueck (Wortlaut hier bewusst nicht wiedergegeben, siehe Hinweis am Dateikopf) | 19.09. |
| Das Geraet brach nach exakt 30 s ab | Board-Log, Abstand `Starting local transcription` zu `Local transcription failed` | belegt — t=605668 bis t=635938, 30 270 ms; `kTranscribeTimeoutMs` war 30 000 | 19.09. |
| Der Server ist langsamer als das Zeitfenster | 9-s-WAV per `curl` an `/api/transcribe-raw`, `time` messen | belegt — 64,96 s. faster-whisper `medium`, int8, 4 Threads auf Pi-5-CPU laeuft weit ueber Echtzeit | 19.09. |
| Der Abbruch traf eine laufende, gelungene Transkription | Server-Log gegen Geraete-Fehler halten | belegt — Server rechnete zu Ende und antwortete 200, das Geraet hatte die Verbindung da schon aufgegeben: `Failed fetching local transcription response headers` | 19.09. |
| **Mit 300 s Zeitfenster geht die Transkription durch** | Flashen, aufnehmen, Log lesen | **belegt** — `Local transcription succeeded: chars=88 clip_ms=5040 total_elapsed_ms=28867`, danach `transcript_saved=1`, Datei `/sdcard/recordings/rec_49_626130.txt` | 19.09. |
| Erklaert, warum es mal ging und mal nicht | Laufzeiten vergleichen | belegt — 28,9 s fuer eine 5-s-Aufnahme lagen knapp unter der alten 30-s-Grenze, laengere Aufnahmen darueber. Kein Wackelkontakt, ein Grenzfall | 19.09. |
| Leere Transkripte sind jetzt nachhoerbar statt zu raten | `brain.log` und `~/transcribe_fehlschlaege/` | belegt — Route loggt Bytes/Rate/Kanaele/Bits/Dauer und legt das Audio bei leerem Ergebnis ab | 19.09. |

## Zusammenfassen bricht ab (19.09.2026)

Dieselbe Klasse wie der Abschnitt darueber, nur eine Etage hoeher: nicht
das Transkribieren lief in sein Zeitfenster, sondern das Zusammenfassen.

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Der Abbruch traf eine laufende, fast fertige Antwort | LM-Studio-Log `~/.lmstudio/server-logs/2026-09/2026-09-19.1.log` zur Abbruchszeit lesen | belegt — Lauf 16:30:32, um 16:31:32 `Client disconnected. Stopping generation...`, Generierung stand da bei `n_gen = 154` und lief noch | 19.09. |
| Das Geraet brach nach exakt 60 s ab | Abstand Anfrageeingang zu `Client disconnected` im Server-Log | belegt — 16:30:32 bis 16:31:32, 60 s auf die Sekunde; `kGenerateTimeoutMs` war 60 000 | 19.09. |
| Das Modell ist langsamer als das alte Zeitfenster | `print_timing`-Zeilen im selben Log lesen | belegt — gemma-4-e2b auf Pi-5-CPU: 8,6 Token/s Prompt-Eval, 5,8 Token/s Generierung. Todo-Lauf 266 Prompt-Token + ~180 Antwort-Token = ~31 s + ~31 s, also ueber 60 s | 19.09. |
| Erklaert, warum Notizen durchliefen und Todos nicht | Laufzeiten der drei Laeufe vergleichen | belegt — Notizen 16:23 `total time = 38 489 ms` (180/106 Token) und Todos 16:26 `48 970 ms` (185/163 Token) blieben unter 60 s, der groessere Todo-Lauf 16:30 (266 Token Prompt) nicht. Kein Wackelkontakt, ein Grenzfall | 19.09. |
| Ein einzelner Chunk allein kann das Fenster sprengen | Prompt auf Chunk-Budget (2500 Token) aufgeblasen, per `curl` an `/v1/chat/completions`, `time` messen | belegt — Lauf ueberschritt 120 s, ohne fertig zu sein. Ein Budget-Chunk liegt rechnerisch bei ~5 min allein fuer das Prompt-Einlesen, und eine Zusammenfassung besteht aus mehreren Chunks plus Rollup | 19.09. |
| **Mit 900 s Zeitfenster baut und laeuft die Firmware** | `idf.py build`, `idf.py -p /dev/ttyACM0 flash` | belegt — `0x375f40` Bytes, 56 % frei; „Hash of data verified", Board bootet, `Local AI readiness check succeeded: model=google/gemma-4-e2b http=200` | 19.09. |
| **Eine volle Zusammenfassung am Geraet geht mit dem neuen Fenster durch** | Am Geraet Todos zusammenfassen, dann `curl http://192.168.178.75/api/archive/summaries` und das Server-Log dazu | **belegt** — Lauf 16:46:38 bis 16:48:10, `total time = 91 988 ms` (304 Prompt-, 191 Antwort-Token), kein `Client disconnected`. Ueber die alte 60-s-Grenze hinaus und trotzdem fertig geworden; Ergebnis liegt als `todos` mit `generated 16:48`, 5 Quellen, 5 Transkripte vor | 19.09. |
| Eine laufende Zusammenfassung hielt das Geraet nicht wach | `GetAutoSleepBlocker` in `main/device_sleep_runtime.cpp` lesen | belegt — die Funktion kannte Aufnahme, Wiedergabe, Speicher, AP-Modus, Zeitsync und Display, aber keinen Zusammenfass-Lauf. **Behoben:** `BlockerReason::kSummaryRunning`, gespeist aus `request.in_flight` | 19.09. |
| Die Schlafgrenzen sind 180 s und 1800 s, nicht 30 s und 90 s | Bootlog lesen, gegen `sdkconfig.defaults` halten | belegt — Log: `display_sleep_in=180s light_sleep_in=1800s`, passend zu `CONFIG_FOLLOWUP_AUTO_SLEEP_*_TIMEOUT_SECONDS`. Die Werte `30`/`90` in `struct Settings` (`device_sleep_service.h`) sind blosse Vorgaben und werden ueberschrieben — eine frueher hier notierte 90-s-Aussage stuetzte sich faelschlich auf sie und ist damit widerlegt | 19.09. |

## Sprache und Ton der Zusammenfassung (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Die englische Ausgabe kam vom Prompt, nicht vom Modell | Anweisung auf Deutsch umschreiben, sonst nichts aendern, neu zusammenfassen | belegt — dasselbe Modell, dieselben Aufnahmen, Antwort jetzt durchgehend deutsch | 19.09. |
| Der aufmunternde Ton stand ausdruecklich in der Anweisung | Alten Prompt lesen | belegt — „write it in an encouraging and optimistic tone", „celebrates progress and motivates the next steps". Das Modell tat, was dort stand; es war kein Ausrutscher | 19.09. |
| Ohne diese Vorgaben berichtet es statt anzufeuern | Nach dem Flashen Lauf ausloesen, Ergebnis lesen | belegt — Todo-Fassung 17:20 nennt die offenen Aufgaben, dann zwei nuechterne Saetze zu Erledigt-Stand und Blockaden. Keine Anrede, kein Lob, kein Ausrufezeichen | 19.09. |
| Umlaute kommen am Geraet richtig an | Zusammenfassung am Display und ueber `/api/archive/summaries` lesen | belegt — `ue`/`ae`/`oe` erscheinen als Umlaute; `DecodeUtf8Codepoint` in `bitmap_font.cpp` dekodiert UTF-8, der Zeichenbereich `0x20`–`0xFC` deckt sie ab | 19.09. |
| Die Statusmeldungen am Geraet blieben bewusst englisch | `SegmentLabelForKind` gegen `PromptLabelForKind` halten | belegt — getrennte Funktionen: der Prompt ist deutsch, die Oberflaeche unangetastet. Die UI umzustellen war nicht bestellt | 19.09. |

## Schlaf waehrend einer Zusammenfassung (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| **Der Blocker greift waehrend eines echten Laufs** | Lauf ausloesen, Board-Log auf `Sleep blocker changed` lesen | **belegt** — `none -> summary_running` bei t=58055, `summary_running -> display_refresh` bei t=118055. 60 Sekunden lang gehalten, exakt ueber den Lauf | 19.09. |
| Der Schlaf-Zaehler sieht den Blocker auch | Log auf `inactivity countdown` pruefen | belegt — `display_sleep_in=180s light_sleep_in=1800s blocked=1 blocker=summary_running`. `blocked=1` heisst: der Zaehler laeuft nicht weiter | 19.09. |
| Der Blocker haelt nicht laenger als noetig | Uebergang nach dem Lauf pruefen | belegt — faellt unmittelbar nach `Summary todos succeeded` weg, kein Nachhaengen | 19.09. |
| Beide Arten loesen ihn aus | Notiz- und Todo-Lauf im selben Log | belegt — auch der Notiz-Lauf zeigt `summary_running -> none` bei t=37055 | 19.09. |

## Lauf anstossen, ohne am Geraet zu stehen (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| `POST /api/archive/summarize` stoesst denselben Lauf an wie der Knopf | `curl -X POST '.../summarize?kind=todos'`, Board-Log lesen | belegt — `ArchivePortal: Summary requested via portal: kind=todos`, danach `SummaryService: Generating todos summary` aus derselben Warteschlange | 19.09. |
| Ein zweiter Anstoss waehrend eines Laufs kippt nichts um | Anstossen, waehrend schon einer laeuft | belegt — Antwort `{"angenommen":false,"laeuft_bereits":true}`, HTTP 200, der laufende Lauf lief unbeirrt zu Ende. Abgelehnt ist hier kein Fehler, sondern die Warteschlange | 19.09. |
| Die Route aendert keine Aufnahme | `archive_portal.cpp` durchsehen | belegt — ruft ausschliesslich `RequestSummary()`; kein `DeleteRecording`, kein `MarkRecording*`, kein `ResetForFormat` | 19.09. |

## Mitschrift auf dem Pi (19.09.2026)

Die Mitschrift (`imkopfhaben-public/brain/mitschrift.py`) greift jedes
Transkript dort ab, wo es entsteht. Ihr Versprechen: sie darf die
Transkription unter **keinen** Umstaenden umwerfen.

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Sie schreibt beim Entstehen mit, nicht auf Nachfrage | Aufnahme machen, danach `/api/mitschrift` lesen, ohne das Geraet zu fragen | belegt — Eintrag von 16:58 erschien ohne jede Abfrage des Geraets | 19.09. |
| Ein kaputter Ablageort wirft die Transkription nicht um | `ORDNER` auf einen unbeschreibbaren Pfad zeigen lassen, mitschreiben | belegt — kein Wurf, Meldung `[mitschrift] konnte nicht schreiben: FileNotFoundError` im Log, Bestand unveraendert. Das ist das Kernversprechen des Moduls | 19.09. |
| Leere Eingaben legen nichts an | `''`, `'   '` und `None` uebergeben | belegt — kein Wurf, Eintragszahl bleibt bei 8 | 19.09. |
| Eine kaputte Zeile kostet eine Zeile, nicht die Datei | Zwei unparsbare Zeilen anhaengen, dann lesen | belegt — 8 Eintraege gelesen, die kaputten uebersprungen. Genau der Fall, fuer den JSONL gewaehlt wurde | 19.09. |
| `?seit=` filtert wie beschrieben | Route ohne Filter, mit Vergangenheits- und mit Zukunftswert | belegt — 8 / 8 / 0 | 19.09. |
| Der Nachtrag traegt nichts doppelt nach | `mitschrift-nachholen.py` zweimal laufen lassen | belegt — zweiter Lauf: „0 nachgetragen, 8 waren schon da". Der erste Anlauf verglich auf den Zeitpunkt und erzeugte eine Dublette (Geraet stempelt den Aufnahmebeginn, die Mitschrift das Ende der Transkription); seither wird nur der Wortlaut verglichen | 19.09. |

## Notizinhalte gehoeren nicht auf GitHub (19.09.2026)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| In keiner getrackten Datei steht ein Notizinhalt | Jede 3-Wort-Folge aus **allen** Aufnahmen der Mitschrift gegen den Commit-Stand beider Repos halten (`git grep -ilF`) | belegt — 7 Treffer, alle nachgesehen und alle unverfaenglich: gewoehnliche Wortfolgen wie „auf dem Geraet" oder „die SD-Karte auf" in technischem Zusammenhang, kein Zitat. Eine Einzelwortsuche allein taugt dafuer nicht: sie schlaegt bei Fachwoertern wie „Aufnahmen" 67-mal an, ohne dass ein Notizinhalt dasteht | 19.09. |
| Auch verdaechtige Einzelwoerter sind harmlos | `Lautstaerker` und `Notizbuch` im Zusammenhang lesen | belegt — „Ausgabe-Lautstaerkeregelung" beschreibt die Hardware, „Notizbuch-Repo" ist ein Projektname. Beide standen schon vor den Aufnahmen da | 19.09. |
| Die Doku-Stellen, die ich selbst eingetragen hatte, sind weg | `git grep` nach den vier bekannten Zitaten | belegt — keine Treffer mehr im Arbeitsstand beider Repos | 19.09. |
| Der `.gitignore`-Riegel greift auf Geraeteinhalte | `git check-ignore -v --stdin` mit acht Beispielpfaden, ohne Dateien anzulegen | belegt — alle acht ignoriert, je mit Regelzeile: `transkripte*`, `aufnahmen/`, `mitschrift/`, `notizen/`, `zusammenfassungen/`, `archive-*.json`, `recordings-*.json`, `*.wav` | 19.09. |
| Der Riegel faengt keine Quelldateien mit ein | Dieselbe Pruefung mit `docs/PRUEFUNG.md`, `main/app_shell.cpp`, `brain/mitschrift.py` | belegt — keine davon ignoriert. Ein zu grober Riegel waere schlimmer als keiner | 19.09. |
| Die Ablage liegt ausserhalb der Repos | Ort der Mitschrift pruefen | belegt — `~/imkopfhaben-mitschrift/`, in keinem Repo enthalten. Der Ablageort ist der erste Riegel, `.gitignore` der zweite | 19.09. |
| **Ein Zitat steht bereits auf GitHub** | `git grep` im Stand von `origin` | **widerlegt fuer die Historie** — `docs/PRUEFUNG.md` Zeile 68 im Commit `3697c8a` enthaelt einen Transkript-Wortlaut. Lokal entfernt, auf GitHub steht er noch. Bereinigung braucht History-Rewrite und Force-Push: **Entscheidung des Besitzers, nicht eigenmaechtig gemacht** | 19.09. |

## Startskript auf Kraken (brain)

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| `imkopfhaben-start.sh` kehrte nie zur Konsole zurueck | Skript starten, `ps --ppid` pruefen | belegt — Subshell wartete auf uvicorn, Skript hing nach „bereit" ueber 10 min; Dienste liefen dabei laengst | 19.09. |
| Nach dem Fix kehrt es zurueck | `timeout 120 ./imkopfhaben-start.sh; echo $?` | belegt — `EXIT=0`, beide Dienste bereit gemeldet | 19.09. |
| `lms load` scheiterte direkt nach `lms server start` | Skript auf kaltem Server starten | belegt — `Text file busy`, Abbruch; jetzt 5 Versuche mit 5 s Abstand | 19.09. |
| `/api/transcribe-raw` nimmt einen rohen WAV-Body an, wie die Firmware ihn schickt | 1-s-Sinus-WAV per `curl --data-binary` an die LAN-IP | belegt — HTTP 200, `{"transcript":""}` (leer ist korrekt, ein Sinuston ist keine Sprache) | 19.09. |

## Taugt der Test etwas? (Gegenprobe)

Ein Test, der nicht fehlschlagen kann, belegt nichts. Die Suite wurde
**nach** dem Code geschrieben, lief also nie gegen das alte Verhalten —
deshalb nachgeholt:

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Die Suite schlägt gegen den **alten** Zustand an | Modell auf eine Schwelle und sofortigen Flush zurückgesetzt | belegt — 7 Fälle rot, darunter „20 Runden mit Pause: Blitz in der Bewegung = ja" und „200 Partials: 22 erzwungene Blitze" statt 3. Genau das gemeldete Symptom | 19.09. |
| Verstellen der weichen Schwelle fällt auf | Mutation 8 → 9 | belegt — erkannt | 19.09. |
| Verstellen der harten Grenze fällt auf | Mutation 60 → 61 und 60 → 59 | belegt — beide erkannt. **War zuvor widerlegt:** die Erwartung stand als `kMaxPartialRefreshesHardCap + 1` da und passte sich jeder Änderung selbst an. Jetzt als Zahl festgenagelt | 19.09. |
| Verschobene Vergleichsgrenzen fallen auf | Mutation `>=` → `>` und `<` → `<=` | belegt — beide erkannt | 19.09. |
| Auseinanderlaufen von Test und Treiber fällt auf | im Klon Treiber auf 12 setzen, Test unverändert lassen | belegt — Abbruch mit „ABGLEICH FEHLGESCHLAGEN: weiche Schwelle ist im Code 12, im Test 8" | 19.09. |

Der Abgleich läuft bei jedem Testaufruf mit: weicht eine Konstante im
Treiber von der im Testmodell ab, bricht das Skript ab, statt still etwas
anderes zu bestätigen als das, was auf dem Gerät läuft.

## Was bewusst ungeprüft blieb

- **Ghosting nach längerem Scrollen.** Ob das Bild bei aufgeschobenem
  Flush sichtbar verblasst, ist eine Frage des Augenmaßes am Gerät und
  hängt am Urteil des Besitzers. Stellschraube:
  `kMaxPartialRefreshesHardCap` in `ssd1677_driver.cpp`.

## Wie der Hardware-Nachweis gefuehrt wurde (Handgriff zum Wiederholen)

Im Leerlauf erzeugt das Gerät nur ~4 Partials (Statuszeile beim Boot), und
der Displayschlaf setzt den Zähler nach 180 s zurück — die Schwelle von 8
ist im Ruhezustand also nie erreichbar. Der Pfad wurde deshalb mit einer
**temporär auf 3 gesenkten Schwelle** bewiesen und danach zurückgebaut:

```sh
# beweisen
sed -i 's/kMaxPartialRefreshesBeforeFlush = 8/kMaxPartialRefreshesBeforeFlush = 3/' \
    components/epaper_panel/ssd1677_driver.cpp
idf.py build && idf.py -p /dev/ttyACM0 flash
# Log auf "Deferred ghosting flush" prüfen, dann zurückbauen und neu flashen
```

Der Rückbau ist gegengeprüft: mit Schwelle 8 erscheint die Zeile nicht
mehr, das Gerät bootet normal.

## Blick von aussen auf Aufnahmen und Zusammenfassungen (19.09.2026)

Neue Lese-Routen in `components/archive_portal`, eingehaengt neben den
schon bestehenden Portal-Routen. Zweck: den Inhalt pruefbar machen, ohne
ihn vom E-Paper abzutippen.

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Die Aufnahmen sind von aussen lesbar | `curl http://192.168.178.75/api/archive/recordings` | belegt — HTTP 200, `count: 7`, jede Aufnahme mit Tag, Dauer, Datum, Flags und Transkripttext | 19.09. |
| Die Zaehlung stimmt mit den gelieferten Eintraegen ueberein | `counts` gegen die Liste halten | belegt — 7 Aufnahmen, davon 2 `idea` und 5 `task`, 2 mit `follow_up`, 0 erledigt; die Liste enthaelt genau diese | 19.09. |
| `?transcripts=0` spart die Transkript-Lesevorgaenge | `curl '.../recordings?transcripts=0'` | belegt — `transcripts_included: false`, kein `transcript`-Feld an den Eintraegen | 19.09. |
| Ein Lesefehler ist von einem leeren Archiv unterscheidbar | Antwortfelder pruefen | belegt — `ok` und `status` tragen den `esp_err_t` der Kartenlesung, unabhaengig von `count` | 19.09. |
| Die Zusammenfassungen sind von aussen lesbar | `curl http://192.168.178.75/api/archive/summaries` | belegt — HTTP 200, `notes` und `todos` je mit Volltext und Herkunft (`source_item_count`, `transcript_item_count`, `truncated`, `chunked`, `window_days`) | 19.09. |
| Die Route zeigt den frischen Stand, nicht den Stand vom Booten | Am Geraet neu zusammenfassen, dann Route lesen | belegt — `RefreshCachedSummaries()` laeuft vor dem Ausliefern; der 16:48-Lauf erschien ohne Neustart | 19.09. |
| Der zusaetzliche Handler-Platz reicht | `config.max_uri_handlers` von 24 auf 28, flashen, alle Routen abfragen | belegt — keine `Failed to register archive route`-Zeile im Log, WLAN-, Zeit- und KI-Routen antworten weiterhin | 19.09. |
| Die Routen aendern keine Aufnahme | `archive_portal.cpp` durchsehen | belegt — nur `HTTP_GET` plus das anstossende `POST /summarize`; `DeleteRecording`, `MarkRecording*` und `ResetForFormat` werden nirgends aufgerufen | 19.09. |
| Falsche HTTP-Methoden prallen ab | `POST` auf `/recordings`, `GET` auf `/summarize`, `DELETE` auf `/recordings` | belegt — alle drei HTTP 405. Eine Leseroute nimmt kein Schreiben an und umgekehrt | 19.09. |
| `?transcripts=` verhaelt sich wie dokumentiert | Sieben Varianten durchprobieren | belegt — ohne Query, `=1`, `=ja` und ein unbekannter Parameter liefern Transkripte; `=0`, `=false`, `=nein` lassen sie weg. Ausgewertet wird das erste Zeichen (`0`/`f`/`n`), das ist die ganze Regel | 19.09. |
| Die Zusammenfassung gibt jede Quelle wieder | Jede Todo-Aufnahme mit ihren Stichworten in der Fassung suchen | belegt — 5 von 5 wiederzufinden, jede mit mehreren Stichworten. Keine Aufnahme unter den Tisch gefallen | 19.09. |
| Die Herkunftsangabe stimmt mit dem Archiv ueberein | `metadata.source_item_count` gegen die gezaehlten Todo-Aufnahmen halten | belegt — Fassung sagt 5 von 5, Archiv hat 5 Todo-Aufnahmen, alle mit Transkript | 19.09. |
| Ton und Sprache halten auch messbar | Beide Fassungen auf Ausrufezeichen, englische Brocken und Umlaute pruefen | belegt — je 0 Ausrufezeichen, keine englischen Wortbrocken, Umlaute vorhanden. Nicht nur gelesen, sondern gezaehlt | 19.09. |

**Befund aus dem ersten Blick auf den Inhalt, nicht gefixt (Vorschlag):**
Die Zusammenfassungen kommen **auf Englisch** zurueck, obwohl jede Quelle
deutsch ist — die Prompts in `summary_service.cpp`
(`BuildSummaryInstructionText`) sind englisch formuliert und das Modell
antwortet in der Sprache der Anweisung. Dazu ein Ton, der nicht
zusammenfasst, sondern den Leser anfeuert — Ausrufezeichen, Lob und
Motivationsformeln statt einer nuechternen Liste. Beides steckt im
Prompt, nicht im Modell,
und waere dort zu aendern. Nicht bestellt, deshalb nicht gebaut.

## Startseite: Sprueche durch Kennzahlen ersetzt (19.09.2026)

Statt der rotierenden Sprueche (`What's on your mind?` etc.) zeigt die
Startseite jetzt einen Statusblock: eigene IP, Brain-Host,
Verbindungszustand zum Brain, Brain-Uptime, ESP32-Chip-Temperatur,
Pi-5-Temperatur und Pi-CPU-Last. Werte, die fehlen, werden ausgelassen.

Neu: Komponente `components/device_status_service` (interner S3-Temp-Sensor
+ Abruf von `/api/status` am Brain), Statusblock in
`components/epaper_ui/welcome_message.cpp` (`info_lines`), Befuellung in
`main/dashboard_page_coordinator.cpp` (`BuildInfoLines`), Hintergrund-Task
`DeviceStatusTask` in `main/app_shell.cpp` (30-s-Takt, blockierendes HTTP
bewusst nicht im UI-Task).

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Firmware baut mit den Aenderungen | `idf.py build` | belegt — `folloup_sticky.bin` 0x378120 Bytes, 56 % frei | 19.09. |
| Brain liefert die Kennzahlen | `python -c "import pi_status,json; print(json.dumps(pi_status.status()))"` (imkopfhaben-public/brain) | belegt — `pi_uptime_sekunden` 21876, `pi_temperatur_celsius` 46.9, `cpu_last_prozent` 5.3, `ram_prozent` 22.2 | 19.09. |
| Die Route `/api/status` ist registriert | `python -c "import main; print([r.path for r in main.app.routes])"` | belegt — `/api/status` in der Routenliste, Import ohne Fehler | 19.09. |
| Sensor-Ausfall ist nicht fatal | `pi_status.py` durchsehen: jede Funktion faengt ab und gibt None | belegt — Statusroute setzt fehlende Werte auf `null`, ESP32 laesst die Zeile weg | 19.09. |
| Auf der Hardware sichtbar (IP/Temp/Uptime stimmen) | `idf.py -p /dev/ttyACM0 flash`, Startseite ansehen, Werte gegen `hostname -I` / `vcgencmd measure_temp` / `uptime` halten | **offen** — Board beim Bauen aus (`imkopfhaben.local` nicht erreichbar), nicht geflasht | 19.09. |
| Live-Route am laufenden Brain antwortet | Dienst neu starten, `curl http://127.0.0.1:8000/api/status` | **offen** — laufender Dienst hat den alten Code, nicht eigenmaechtig neu gestartet | 19.09. |
| Statusblock draengt die Menuepunkte nicht nach unten | Startseite am Geraet ansehen | belegt — nach Umbau auf zwei Spalten (kleinste Schrift 22 px, kurze Labels) stehen die Werte in zwei Spalten unter dem Datum, Todos wieder an gewohnter Stelle | 19.09. |
| Board erreicht /api/status nach Dienst-Neustart | Boot-Log 40 s lesen, auf `Brain-Status HTTP` achten | belegt — 0 Fehlerzeilen (404 verschwand nach Neustart von imkopfhaben-brain), Route liefert HTTP 200 | 19.09. |

## Lautstaerkeregler in Advanced (20.09.2026)

Neuer Eintrag „Volume" unter den drei Advanced-Buttons. Anwaehlen, Klick
schaltet in den Einstell-Modus, Up/Down stellen die Lautstaerke in Stufen
(0/25/50/75/100), 0 % ist Mute, Kontrollton bei jeder echten Aenderung,
nochmal Klick zurueck in den Auswahlmodus. Wert liegt in NVS (`audio/out_vol`,
Default 50) und wird beim Boot in `waveshare_board::GetAudioCodec()` gesetzt.
Neuer Dienst `components/audio_settings_service`.

| Behauptung | Handgriff | Ergebnis | Datum |
|---|---|---|---|
| Firmware baut mit den Aenderungen | `idf.py build` | belegt — `folloup_sticky.bin` 0x378a70 Bytes, 56 % frei | 20.09. |
| Firmware laeuft auf dem Board | `idf.py -p /dev/ttyACM0 flash` | belegt — „Hash of data verified", hard reset | 20.09. |
| Gespeicherte Lautstaerke wird beim Boot geladen und angewendet | Boot-Log lesen | belegt — `audio_settings: Lautstaerke geladen: 50 %`, danach `AudioCodec: Set output volume to 50`, `Set output mute to false` | 20.09. |
| Up/Down verstellt die Lautstaerke mit Kontrollton | Am Geraet: Advanced → Volume → Klick → Up/Down | belegt — Besitzer bestaetigt: Ton wird abgespielt | 20.09. |
| 0 % ist wirklich still | Bis auf Mute herunterstellen | belegt — Besitzer bestaetigt: „mute is ruhig" | 20.09. |
| Letzter Wert ueberlebt den Neustart | Wert aendern, Board neu starten, Boot-Log pruefen | belegt — Besitzer stellte auf 25 %, Aus/Ein: `audio_settings: Lautstaerke geladen: 25 %`, `Set output volume to 25` | 20.09. |
