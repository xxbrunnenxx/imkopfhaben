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
