# Versionen — folloup-waveshare

Stände, die am Gerät liefen und als brauchbar befunden wurden. Ein Tag
wird gesetzt, wenn der Besitzer den Stand im Betrieb geprüft hat — nicht,
wenn er nur baut.

## v0.1 — ruhiges Display (19.09.2026)

Erster Stand, bei dem das Panel im normalen Gebrauch nicht mehr blitzt.
Besitzer-Urteil nach längerer Nutzung: **„bislang nix festgestellt."**

**Was diesen Stand ausmacht**

- **Screenwechsel laufen auf der schnellen Wellenform.** Vorher fuhr jeder
  Seitenwechsel die mode-1-Vollwellenform: 2,11 s hartes Blinken pro
  Navigationsschritt. Jetzt 1,71 s auf `kFast`; jeder achte Wechsel bleibt
  voll, damit sich kein Ghosting aufbaut. Eingriff zentral in
  `display_service::SetCurrentScreen`, nicht an den 20 Aufrufstellen.
- **Die Ghosting-Auffrischung wartet auf den Leerlauf.** Das war die
  eigentliche Ursache des gemeldeten Blitzens: nach 8 Teilbildern erzwang
  der Treiber sofort einen 2,1-s-Voll-Refresh — beim Scrollen also mitten
  in der Bewegung. Jetzt markiert die Schwelle den Flush nur als fällig;
  gefahren wird er, wenn die Kommando-Queue 2,5 s ruhig bleibt. Eine harte
  Grenze bei 60 Teilbildern schützt weiter gegen Verblassen.
- **Ein ruhiges Gerät wird nicht geweckt.** Der begrenzte Queue-Wait ist
  nur scharf, solange ein Flush aussteht; sonst blockiert `DisplayTask`
  unbegrenzt wie zuvor.

**Gemessen am Gerät**

| | vorher | v0.1 |
|---|---|---|
| Screenwechsel | 2,11 s | 1,71 s |
| Scrollschritt | 0,51 s | 0,51 s |
| Blitz mitten im Scrollen | ja | keiner |

**Belegt** in `docs/PRUEFUNG.md` (26 Zeilen, alle belegt) und maschinell
durch `scripts/flush-politik-test.sh` (23 Fälle). Die Suite fällt gegen den
alten Zustand in 7 Fällen durch, taugt also als Regressionsschutz.

**Bekannte Lücken dieses Standes**

- **Der Light-Sleep-Button-Fix von `main` fehlt hier** (Commit `70e7ed7`).
  Nach dem Aufwachen aus dem Light-Sleep wird ein Doppelklick auf die
  Power-Taste als einfacher Klick gemeldet, die Entsperrgeste im Uhrmodus
  greift dann nicht. Ursache dort beschrieben: `iot_button` spielt nach dem
  Aufwachen tausende verpasste Timer-Ticks in Mikrosekunden nach. Der Zweig
  ist von `main` abgezweigt, bevor das behoben wurde. **Nicht eingebaut,
  weil nicht bestellt** — als Vorschlag vermerkt.
- **Ghosting bei sehr langem Scrollen** ist nicht durch Messung
  entschieden, sondern Augenmaß. Falls das Bild zu blass wird, ist
  `kMaxPartialRefreshesHardCap` in `ssd1677_driver.cpp` die Stellschraube
  (aktuell 60).

**Stand:** Zweig `folloup-waveshare`. Der Zweig liegt 94 Commits vor
`origin/main`; von den 8 Commits auf `main` sind 5 inhaltlich hier
enthalten, 3 nicht (`70e7ed7` oben, `be7f0d9` und `cd44bb2` sind hier
bereits anders gelöst — `sdkconfig` ist auch in diesem Zweig ignoriert und
enthält keine Zugangsdaten).
