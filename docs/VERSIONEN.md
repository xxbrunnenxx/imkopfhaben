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

**Wie die Ursache gefunden wurde** (weil der erste Verdacht falsch war)

Der erste Eingriff galt den Screenwechseln — richtig, aber nicht die
Ursache des gemeldeten Blitzens. Erst ein serieller Mitschnitt *während*
der Besitzer das Gerät benutzte, zeigte es: im ganzen Fenster gab es
keinen einzigen Screenwechsel. Alle Kommandos waren
`mode=partial scope=region source=dashboard_page` — Scrollen mit der
DOWN-Taste. Genau ein Voll-Refresh, `busy=2108938us` bei t=119264, direkt
nach sechs Partials à ~510 ms.

Das war der Beleg, dass der erzwungene Flush im Partial-Pfad schuld war
und nicht der Screenwechsel. Die `refresh metrics` des Treibers trennen
die drei Fälle eindeutig und taugen deshalb als Messinstrument:

| Waveform | `busy` |
|---|---|
| voll (mode-1) | ~2 109 000 µs |
| schnell (OTP) | ~1 709 000 µs |
| partial | ~510 000 µs |

Merksatz: bei einer Beschwerde über das Anzeigeverhalten zuerst
mitschneiden, während das Gerät benutzt wird. Der Log nennt Waveform,
Bildschirm und Auslöser pro Ansteuerung; Raten kostet mehr Zeit als Messen.

**Nachtrag 19.09.2026 — zwei Fehler beim ersten Anschluss an Kraken**

Nach dem Taggen wurde das Board erstmals gegen die laufenden Kraken-Dienste
gehalten. Der KI-Stern in der Statusleiste, der „verbunden mit Brain und
Sprachmodell" anzeigt, blieb aus. Zwei Ursachen, beide behoben und in
`docs/PRUEFUNG.md` belegt:

1. **Veraltete Serveradresse im NVS, unerreichbar zum Korrigieren.** Das
   Board fragte `http://192.168.0.146:1234/v1/` ab, Kraken liegt unter
   `192.168.178.215` (`kraken.local`). Schlimmer als der falsche Wert war,
   dass er sich nicht ändern ließ: `StartConfigPortal()` lief **nur im
   AP-Modus**, und auf der Einstellungsseite des Geräts gibt es kein Feld
   dafür. Ein Gerät im WLAN hatte damit keinen Weg zu seinen eigenen
   Einstellungen. Die Weboberfläche läuft jetzt auch im Stationsbetrieb
   unter der DHCP-Adresse des Geräts; der Captive-DNS bleibt dem AP-Modus
   vorbehalten, damit im Heimnetz nichts mit dem Router konkurriert.
2. **Die Readiness-Prüfung lief nur einmal pro WLAN-Ereignis.** War der
   lokale KI-Server beim Verbinden noch nicht oben, blieb der Stern bis zum
   nächsten Neustart aus — der Normalfall, weil die Kraken-Dienste bewusst
   von Hand gestartet werden. Jetzt läuft alle 60 s ein flacher
   Wiederholungsversuch, solange nicht verbunden. Live belegt: Server aus →
   `ready=false`, Server hoch, 70 s warten → `ready=true` ohne Neustart und
   ohne WLAN-Ereignis.

Auf Kraken fielen dabei zwei Fehler im Startskript auf
(`imkopfhaben-public`, Commit `2953a54`): es kehrte nie zur Konsole zurück,
weil die Subshell auf uvicorn wartete, und es brach mit „Text file busy" ab,
wenn `lms load` zu dicht auf `lms server start` folgte.

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

**Zum Board-Stand beim Taggen:** Die Firmware dieses Standes ist per
`verify_flash` gegen das Board bestätigt („digest matched"), im Betrieb
laufen nur Teilbild-Refreshes à 0,51 s, kein Voll-Refresh, kein Absturz.

Direkt nach dem Taggen schwieg die serielle Ausgabe eine Weile. Erste
Vermutung war der Light-Sleep nach 1800 s — **das war falsch.** Das Board
hing im Bootloader, nachdem DTR/RTS-Reset-Versuche über die USB-JTAG-Brücke
mit `Errno 71` abgebrochen waren. Ein `--after hard_reset` startet es
wieder. Merksatz fürs nächste Mal: bei plötzlicher Stille am ESP32-S3 mit
USB-JTAG zuerst `esptool ... chip_id` fragen — meldet es „Staying in
bootloader", ist das die Erklärung, nicht der Schlafmodus.

**Stand:** Zweig `folloup-waveshare`. Der Zweig liegt 94 Commits vor
`origin/main`; von den 8 Commits auf `main` sind 5 inhaltlich hier
enthalten, 3 nicht (`70e7ed7` oben, `be7f0d9` und `cd44bb2` sind hier
bereits anders gelöst — `sdkconfig` ist auch in diesem Zweig ignoriert und
enthält keine Zugangsdaten).

## Nächster Einstieg (Stand 19.09.2026, 10:18)

Alles erledigt und gepusht. `v0.1` = `b0306dd`, Board trägt genau diesen
Build, Arbeitsbaum sauber, Graph aktuell.

Zwei Dinge liegen bereit, beide **nicht bestellt**, also nicht gebaut:

1. **Light-Sleep-Button-Fix von `main` fehlt** (`70e7ed7`). Doppelklick
   auf die Power-Taste wird nach dem Aufwachen als einfacher Klick
   gemeldet, Entsperrgeste im Uhrmodus greift dann nicht.
   Handgriff: `git cherry-pick 70e7ed7`, bauen, flashen, am Gerät prüfen.
2. **Ghosting bei sehr langem Scrollen** — Geschmacksfrage, braucht das
   Auge des Besitzers. Stellschraube `kMaxPartialRefreshesHardCap` in
   `components/epaper_panel/ssd1677_driver.cpp` (60, kleiner = häufiger).

Vor jeder Änderung: `./scripts/flush-politik-test.sh` (läuft ohne Board).
