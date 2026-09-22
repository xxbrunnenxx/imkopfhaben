# live-test-hampelmann - Wiederaufnahme (Stand 22.09. vormittags)

## Wo wir stehen
Der Live-Test laeuft weiter, ist NICHT abgeschlossen - wird woanders fortgesetzt.
Board geflasht mit dem aktuellen Stand von `live-test-hampelmann-fixes`.
Bisher drei Rueckmeldungen abgearbeitet und vom Besitzer live bestaetigt,
alle committet + in PRUEFUNG.md belegt.

## Erledigt auf diesem Zweig (committet)
- `6a2f2e1` 420-Track im Menue-Stil (obere Trennlinie, Zeile 420-Track,
  Punkte drunter, invertiert wie Menue; `counting` von `focused` getrennt)
  + Startseiten-Abstaende gestrafft (k32/k48/k12/k8 -> k16/k16/k4/k2).
- `ef5eeeb` FN-Langdruck (Rotary-Druck) fuehrt aus jedem Menue zur Startseite,
  Lockscreen ausgenommen, ACTION/Aufnahme unberuehrt.

Rueckmeldungen 1+2 in `rueckmeldungen.md` stehen auf ERLEDIGT. Der
Summarize-"Deadlock" war ein Fehlbefund (Verlassen geht per DOWN-Geste),
kein Code geaendert.

## Noch offen (fuer die naechste Sitzung woanders)
- Test ist nicht durch: weiter Rueckmeldungen am Board sammeln und abarbeiten.
- Zweig `live-test-hampelmann-fixes` liegt VOR `folloup-waveshare`; enthaelt
  noch den alten WIP-Commit `0ecf727` ("baut noch nicht") als ersten Commit.
  Vor einem PR: die WIP-Historie ueberdenken (ggf. squashen). NICHT ohne
  Besitzer-OK mergen.

## Wie flashen (Kraken)
```
cd ~/imkopfhaben-esp32
source ~/esp-idf/export.sh
idf.py build && idf.py -p /dev/ttyACM0 flash
```
Board haengt an /dev/ttyACM0.

## Serial-Mitschnitt
Logger: `~/.jcode/scratch/hampelmann/serial_logger.py` (auto-reconnect, ttyACM0).
Log (fluechtig): `~/.jcode/scratch/hampelmann/serial.log`.
