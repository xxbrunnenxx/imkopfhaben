# live-test-hampelmann - Wiederaufnahme (Stand 21.09. 20:08, vor Token-Tod)

## Wo wir stehen
Board geflasht mit folloup-waveshare (Todos-Fokus-Fix #12 + Monkey-Test #13).
Live-Test laeuft, Besitzer meldet Bugs, wir fixen. Zwei offene Punkte, siehe
`rueckmeldungen.md`. An Punkt 1+2 wurde ANGEFANGEN - NICHT fertig, NICHT geflasht.

## Was schon geaendert ist (uncommittet -> in diesem WIP-Commit)
- `components/epaper_ui/include/epaper_ui/joint_tracker_card.h`:
  Neues Feld `counting` (getrennt von `focused`), Stil auf Menue-Optik umgestellt
  (selected_background/​selected_text/​border, top_border statt corner_radius,
  label_role auf kLabelMediumBlack wie Menue). **.cpp ist noch NICHT angepasst
  -> baut aktuell NICHT** (Felder border_thickness/corner_radius/status_bar
  entfernt, joint_tracker_card.cpp nutzt sie noch).

## Naechste Schritte (morgen, in Reihenfolge)
1. `joint_tracker_card.cpp` neu zeichnen im Menue-Stil:
   - obere Trennlinie (top_border), darunter Zeile "420-Track"
   - Zeile invertiert wenn `focused` (schwarz/weiss wie ausgewaehlte Menuezeile)
   - im `counting`-Modus wieder zurueck-invertiert (weiss), Punkte fuellbar
   - Punkte in der Zeile DRUNTER
2. `main/dashboard_page_coordinator.cpp` Z.260: `focused`/`counting` getrennt setzen
   (focused = IsJointTrackerFocused(); counting = joint_tracker_counting_).
3. Punkt 1 (Startseite Abstaende zu gross): in `dashboard_page.cpp`
   kContentTopGap/kWelcomeMiddleGap/kTrackerTopGap/kTrackerMenuGap straffen,
   ggf. Menue-Zeilenhoehe lokal kleiner (Token menu_item::kHeight=72 ist geteilt,
   NICHT global aendern - lokal in MenuStyle setzen).
4. `idf.py build && idf.py -p /dev/ttyACM0 flash`, dann live weiter mit Besitzer.
5. Wenn gruen: PRUEFUNG.md nachziehen, PR gegen folloup-waveshare.

## Serial-Mitschnitt
Logger: `~/.jcode/scratch/hampelmann/serial_logger.py` (auto-reconnect, ttyACM0).
Log (fluechtig): `~/.jcode/scratch/hampelmann/serial.log`.
