# Followup Produktvorstellung

Followup ist ein Ort, um deine Gedanken festzuhalten — egal ob Idee, To-do oder einfach eine Notiz. Nimm auf, was dir gerade durch den Kopf geht, im Moment der Eingebung, bevor es wieder verschwindet, und Followup hilft dir danach, es zu ordnen. Mit einem lokalen KI-Server im eigenen Netzwerk werden deine Aufnahmen automatisch transkribiert und zusammengefasst. Alles wird auf deiner SD-Karte gespeichert.

Followup läuft auf dem [Waveshare ESP32-S3-ePaper-3.97](https://docs.waveshare.com/ESP32-S3-ePaper-3.97) — deine Gedanken leben also auf einem ruhigen, dauerhaft eingeschalteten Bildschirm, den du überall hinstellen kannst: eine ständige, unaufdringliche Erinnerung statt einer weiteren Benachrichtigung, die im Handy untergeht.

## Positionierung in einem Satz

**Followup ist ein sprachbasierter Begleiter fürs Festhalten von Gedanken auf einem dauerhaft eingeschalteten ePaper-Display: nimm Ideen, To-dos und Notizen im Moment auf, lass sie von einem lokalen KI-Server transkribieren und zusammenfassen, und behalte die wichtigen als Klebezettel im Blick.**

## Wofür es geeignet ist

- Eine plötzliche Idee per Sprache festhalten, im Moment der Eingebung, bevor sie vergessen ist
- Schnelle To-dos und Notizen freihändig festhalten, während man gerade mit etwas anderem beschäftigt ist
- Eine kleine, dauerhaft sichtbare Sammlung an Follow-ups auf dem Schreibtisch, am Kühlschrank oder an der Wand
- Alte Ideen später erneut ansehen, um zu entscheiden, was noch verfolgenswert ist
- Für alle, die ihre Gedanken geordnet haben wollen, ohne dafür in einer weiteren Handy-App zu leben

## Kernfunktionen

### 1. Festhalten im Moment der Eingebung

Aufnahme drücken und sprechen. Jede Aufnahme beginnt als Sprachmitschnitt, getaggt als **Idee**, **To-do** oder **Notiz** — der Gedanke ist so in dem Moment festgehalten, in dem er kommt, ohne zum Tippen anzuhalten.

### 2. Lokale KI-Transkription und -Zusammenfassung

Sobald eine Aufnahme gespeichert ist, transkribiert und fasst ein lokaler KI-Server im eigenen Netzwerk das Audio zusammen — aus einer ausschweifenden Sprachnotiz wird lesbarer Text und eine knappe Zusammenfassung, die sich auf einen Blick erfassen lässt.

Ein lokaler KI-Server im selben Netzwerk ist dafür erforderlich (dieser Fork zielt auf einen Heimserver mit LM Studio, google/gemma-4-e2b für Zusammenfassungen und faster-whisper für die Transkription — siehe docs/local-ai-service.md). Kein Cloud-Konto, kein API-Key, keine Nutzungsgrenzen.

### 3. Alles auf deiner SD-Karte gespeichert

Aufnahmen, Transkripte und Zusammenfassungen werden lokal auf der SD-Karte des Geräts gespeichert. Deine Gedanken bleiben bei dir, auf deinem eigenen Speicher.

### 4. Vibe-Check für deine Ideen

Nicht jede Idee altert gut. Sieh dir jede einzelne an und entscheide, ob sie noch einen Vibe hat, den es zu behalten lohnt — oder ob sie in den Müll kann, damit du mit klarem Kopf weitermachen kannst.

### 5. Follow-up für Aufgaben und Notizen

Markiere eine Aufgabe oder Notiz als Follow-up, um sie im Blick zu behalten. Followup hilft dir, den Überblick zu behalten und dich auf das zu konzentrieren, was als Nächstes wirklich ansteht.

### 6. Deine Follow-ups als Klebezettel anzeigen

Pinne deine Follow-ups als Klebezettel auf das ePaper-Display. Weil der Bildschirm dauerhaft eingeschaltet und stromsparend ist, bleiben sie als ständige, sanfte Erinnerung vor dir.

## Typische Anwendungen

| Anwendung | Beschreibung |
| --- | --- |
| Idee | Einen Einfall per Sprache festhalten und später mit einem Vibe-Check erneut ansehen |
| To-do | Eine Aufgabe freihändig aufnehmen und dranbleiben, bis sie erledigt ist |
| Notiz | Einen schnellen Gedanken oder eine Erinnerung festhalten, transkribiert und zusammengefasst |
| Follow-up | Die Dinge markieren, die wichtig sind, damit sie im Kopf bleiben |
| Klebezettel | Die aktiven Follow-ups auf dem ePaper als dauerhafte Erinnerungen anzeigen |
| Zusammenfassungen | Den lokalen KI-Server lange Aufnahmen zu einer überschaubaren Zusammenfassung verdichten lassen |

## Kurzspezifikationen

Followup läuft auf dem [Waveshare ESP32-S3-ePaper-3.97](https://docs.waveshare.com/ESP32-S3-ePaper-3.97).

| Punkt | Angabe |
| --- | --- |
| Produktname | Followup (auf ESP32-S3-ePaper-3.97) |
| Produkttyp | Sprachbasierte Notiz-App auf einem ePaper-Terminal |
| MCU | ESP32-S3R8, Dual-Core Xtensa LX7 bis 240MHz |
| Speicher | 8MB PSRAM, 16MB Flash |
| Bildschirm | 3,97-Zoll Schwarz-Weiß-ePaper, 800 x 480, SSD1677-Controller |
| Bedienung | Nur Tasten — dieses Board hat keinen Touchscreen (siehe [Bedienelemente](#bedienelemente)) |
| Konnektivität | 2,4GHz WLAN (802.11 b/g/n), Bluetooth 5 (LE) |
| Audio | ES8311-Codec, eingebautes Mikrofon, NS4150B-Verstärker, Lautsprecheranschluss |
| Sensoren | QMI8658 6-Achsen-IMU, PCF85063 Echtzeituhr |
| Stromversorgung | AXP2101 PMIC, 3,7V-Lithium-Akku (MX1.25-Anschluss), USB-C-Laden |
| Speicherkarte | microSD-Karte (Aufnahmen, Transkripte, Zusammenfassungen) |
| KI | Lokaler KI-Server (LAN, keine Cloud) für Transkription und Zusammenfassung, über WLAN |

Das Board trägt außerdem einen SHTC3-Temperatur-/Feuchtigkeitssensor am gemeinsamen I2C-Bus. Followup liest ihn aktuell nicht aus.

## Bedienelemente

Followup wird ausschließlich über die drei physischen Bedienelemente gesteuert: eine Wipptaste, die BOOT-Taste und die PWR-Taste.

| Bedienelement | Aktion |
| --- | --- |
| Wipptaste hoch / runter | Auswahl bewegen; halten zum Wiederholen |
| Wipptaste runter, gehalten | Aus einer Liste oder Karte zurückgehen, in die man hineingegangen ist |
| Wipptaste Mitte | Auswählen / bestätigen |
| BOOT, tippen | Auswählen / bestätigen |
| BOOT, gedrückt halten | Aufnehmen — die Aufnahme beginnt beim Halten und stoppt beim Loslassen |
| PWR, tippen | Bildschirm sperren oder entsperren |
| PWR, ~1s halten | Herunterfahr-Bestätigung öffnen |
| PWR, 6s halten | Hardware-Abschaltung, direkt über den PMIC |

Aufnehmen ist exklusiv der BOOT-Taste vorbehalten, damit kein anderes Bedienelement versehentlich eine Aufnahme starten oder stoppen kann. Das 6-Sekunden-Halten der PWR-Taste umgeht die Firmware komplett und kappt immer die Stromversorgung.

## Produktwert zusammengefasst

Der Wert von Followup ist ein ruhiger, dauerhaft sichtbarer Ort, um Gedanken festzuhalten und die wichtigen im Blick zu behalten. Statt eine Idee in einer vergessenen Notiz-App zu verlieren oder eine Aufgabe in einem Strom von Benachrichtigungen zu vergraben, sprichst du sie im Moment aus, lässt sie von deinem lokalen KI-Server in sauberen Text und eine Zusammenfassung verwandeln und behältst alles privat auf deiner SD-Karte und in deinem eigenen Netzwerk.

Ideen bekommen einen Vibe-Check, damit du nur das weiterträgst, was noch zählt. Aufgaben und Notizen werden zu Follow-ups, damit du dranbleibst. Und die, die dir am wichtigsten sind, liegen als Klebezettel auf dem ePaper — eine stetige, unaufdringliche Erinnerung daran, was als Nächstes ansteht.
