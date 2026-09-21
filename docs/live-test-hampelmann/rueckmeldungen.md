# live-test-hampelmann - Rueckmeldungen

Board geflasht 20:01, Stand folloup-waveshare inkl. Todos-Fokus-Fix (#12) + Monkey-Test (#13).
Nur mitschreiben, NICHT fixen. Nachverfolgen spaeter.

| # | Zeit | Ort | Rueckmeldung | Code-Spur (fuer spaeter) | Status |
|---|------|-----|--------------|--------------------------|--------|
| 1 | 20:02 | Startseite (Dashboard) | Striche und Texte zu weit auseinander (Abstand zu gross) | `components/epaper_ui/dashboard_page.cpp` Layout-Konstanten Z.12-16 (`kContentTopGap`, `kWelcomeMiddleGap`, `kTrackerTopGap`, `kTrackerMenuGap`) + `item_gap`/`bottom_border` Z.69/240 | offen |
| 2 | 20:05 | Startseite -> 420-Track-Box (eins hoch) | Auswahl der 420-Box ist zu unauffaellig. Soll im GLEICHEN Stil wie die anderen Menuetexte werden: (a) horizontale Linie, darunter Zeile "420-Track", (b) in der Zeile drunter die Punkte. (c) Beim Drueberfahren mit dem Regler wird der Eintrag INVERTIERT wie die anderen Texte. (d) Beim Auswaehlen (Zaehlmodus an) wird wieder invertiert und dann koennen die Punkte gefuellt werden. | `components/epaper_ui/joint_tracker_card.*` (Kartendarstellung), Fokus/Invert-Stil in `dashboard_page.cpp` (`item_style`, Menue-Items) analog zu den Menuezeilen; Zaehlmodus-Umschaltung in `main/page_input_runtime.cpp` | offen |
