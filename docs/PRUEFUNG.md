# imkopfhaben-esp32 — Prüfprotokoll

> Angelegt 19.09.2026

Jede Behauptung braucht den **Handgriff, der sie umwerfen würde**. Ist er
nicht gelaufen, gilt die Behauptung als ungeprüft — nicht als wahr.

Die Datei existierte bisher leer. Angelegt wurde sie, als die
Refresh-Änderungen belegt werden mussten.

**Ergebnis-Werte:** `belegt` · `offen` · `widerlegt`

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
| Harte Grenze bei 60 Partials greift | Zähllogik über 100 Partials ohne Pause | belegt — erster erzwungener Voll-Refresh exakt bei Partial 61 (Host-Nachbau, `flush.cpp`) | 19.09. |
| Leerlauf holt den fälligen Flush nach und setzt den Zähler zurück | Zähllogik: 8 Partials, dann Leerlauf | belegt — Flush gefahren, Zähler auf 0 (Host-Nachbau) | 19.09. |
| Leerlauf ohne fälligen Flush blitzt nicht | Zähllogik: 3 Partials, dann Leerlauf | belegt — kein Flush, also kein Blitzen im Ruhezustand | 19.09. |
| Scrollen mit Pausen blitzt nie mitten in der Bewegung | 20 Runden à 8 Partials je mit Pause | belegt — kein einziger Blitz innerhalb einer Runde (Host-Nachbau) | 19.09. |
| Aufgeschobener Flush läuft **auf der Hardware** im Leerlauf nach | siehe unten | **offen** — die Logik ist am Host belegt, der Pfad auf dem Board nie ausgelöst: im Mitschnitt kam vorher stets ein Screenwechsel dazwischen, und im Ruhezustand erzeugt das Gerät zu wenige Partials | 19.09. |

## Was bewusst ungeprüft blieb

- **Ghosting nach längerem Scrollen.** Ob das Bild bei aufgeschobenem
  Flush sichtbar verblasst, ist eine Frage des Augenmaßes am Gerät und
  hängt am Urteil des Besitzers. Stellschraube:
  `kMaxPartialRefreshesHardCap` in `ssd1677_driver.cpp`.

## Der eine offene Handgriff

Die letzte offene Zeile schließt sich mit einer einzigen Handlung am
Gerät. Mitschnitt starten, dann **im Dashboard rund zehnmal scrollen und
danach drei Sekunden nichts tun** — ohne zwischendurch die Seite zu
wechseln, denn ein Screenwechsel frischt ohnehin auf und nimmt dem Flush
die Arbeit weg.

```sh
grep -aE "Deferred ghosting flush" mitschnitt.txt
```

Erscheint die Zeile, ist der Pfad belegt. Erscheint sie nicht, obwohl
zehn Partials liefen, ist die Behauptung **widerlegt** und die
Leerlauf-Erkennung in `DisplayTask` greift nicht.

Die Zähllogik selbst ist am Host bewiesen (`flush.cpp`, fünf Fälle, alle
grün); offen ist allein, ob der 2,5-s-Leerlauf auf dem Board so eintritt
wie gedacht.
