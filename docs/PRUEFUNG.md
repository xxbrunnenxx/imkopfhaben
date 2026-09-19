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
| Harte Grenze bei 60 Partials greift | `scripts/flush-politik-test.sh` | belegt — erster erzwungener Voll-Refresh exakt bei Partial 61 | 19.09. |
| Leerlauf holt den fälligen Flush nach und setzt den Zähler zurück | `scripts/flush-politik-test.sh` | belegt | 19.09. |
| Leerlauf ohne fälligen Flush blitzt nicht | `scripts/flush-politik-test.sh` | belegt — kein Blitzen im Ruhezustand | 19.09. |
| Scrollen mit Pausen blitzt nie mitten in der Bewegung | `scripts/flush-politik-test.sh` | belegt — 20 Runden à 8 Partials, kein Blitz innerhalb einer Runde, 20 Flüsche im Leerlauf | 19.09. |
| Screenwechsel nach 8 Partials lässt den Flush entfallen | `scripts/flush-politik-test.sh` | belegt — kein nachträglicher Blitz. Erklärt, warum im Nutzungs-Mitschnitt nie ein Flush auftauchte | 19.09. |
| Im Displayschlaf wird kein Flush auf das schlafende Panel gefahren | `scripts/flush-politik-test.sh` | belegt — auch kein Altlast-Flush nach dem Aufwachen | 19.09. |
| Ein ruhiges Gerät wird nicht alle 2,5 s geweckt | `scripts/flush-politik-test.sh` | belegt — ohne fälligen Flush blockiert `DisplayTask` unbegrenzt wie zuvor | 19.09. |
| **Aufgeschobener Flush feuert auf der Hardware** | Schwelle temporär auf 3, flashen, Log lesen | **belegt** — `Deferred ghosting flush: idle after 4 partials` bei t=18136, letzter Partial bei t=15636: exakt 2,5 s Ruhe davor. Schwelle danach auf 8 zurückgesetzt, neu geflasht, gegengeprüft (kein Flush mehr) | 19.09. |

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
