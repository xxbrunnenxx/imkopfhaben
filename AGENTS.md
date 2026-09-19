# Agenten-Hinweise

Vor Änderungen an der Firmware-Architektur, Komponenten-Grenzen, der
BQ27220-Integration, der Board-Verkabelung, dem Partitionslayout oder der
ESP-IDF-Konfiguration erst `docs/app-architecture.md` lesen.

`main/app_shell.cpp` bleibt eine Orchestrierungs-Schicht. Bevor dort Logik
ergänzt wird: fragen, ob das Verhalten stattdessen in einen Service/eine
Komponente oder einen fokussierten Runtime-Helfer gehört. `app_shell` darf
die Start-Reihenfolge, Event-Verdrahtung, einfaches Produkt-Routing und
Policy-Komposition übernehmen, soll aber nicht mit Hardware-Treiber-Logik,
Protokoll-Logik, Display-Zeichnen, Power-/Sleep-Mechanik, langlebigen
Feature-Schleifen oder Geschäftslogik wachsen, die eigentlich einem
Service/einer Komponente gehört.

Builds in diesem Repo nicht automatisch ausführen. Weist der Nutzer
ausdrücklich an, einen Build laufen zu lassen: den bestehenden `build/`-
Ordner nutzen, keinen neuen anlegen.

## Wo was steht

| Datei | Wofür |
|---|---|
| `docs/app-architecture.md` | Architektur, Komponenten-Grenzen, Hardware-Anmerkungen. Vor Architektur-Änderungen lesen |
| `docs/VERSIONEN.md` | Getaggte Stände, was sie ausmacht, ihre bekannten Lücken |
| `docs/PRUEFUNG.md` | Prüfprotokoll: zu jeder Behauptung der Handgriff, der sie umwerfen würde. Eine Behauptung ohne Zeile dort gilt als ungeprüft, nicht als wahr |
| `docs/offene-punkte.md` | Was noch offen ist, grob nach Priorität |
| `docs/waveshare-epaper-hardware-spec.md` | Board-Details |
| `docs/local-ai-service.md` | Der KI-Server im Heimnetz |
| `docs/auto-sleep.md` | Schlaf-Stufen und Zeitgrenzen |
| `scripts/flush-politik-test.sh` | Host-Test der Refresh-/Flush-Politik. Läuft ohne Board, gleicht seine Konstanten mit dem Treiber ab und bricht bei Abweichung ab |
| `scripts/archiv-zeigen.py` | Zeigt Aufnahmen und Zusammenfassungen des laufenden Geräts lesbar an. Nur lesend, braucht ein erreichbares Board |

Geprüft wird, statt behauptet. Was nicht gelaufen ist, wird als ungelaufen
gekennzeichnet — auch wenn es sicher aussieht. Wer eine Lücke nennt, nennt
im selben Zug den Befehl, der sie schließt.
