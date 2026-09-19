#!/usr/bin/env python3
"""Zeigt Aufnahmen und Zusammenfassungen des Geraets als lesbaren Text.

Die Routen /api/archive/recordings und /api/archive/summaries liefern
JSON -- gut fuer Werkzeuge, schlecht fuer Augen. Dieses Skript legt beides
nebeneinander, damit sich beurteilen laesst, ob eine Zusammenfassung zu
ihren Aufnahmen passt. Es liest nur.

    scripts/archiv-zeigen.py                  # Geraet unter followup.local
    scripts/archiv-zeigen.py 192.168.178.75   # oder per Adresse
"""

import json
import sys
import urllib.request
from datetime import datetime

STANDARD_HOST = "192.168.178.75"


def hole(host: str, pfad: str, timeout: int = 90) -> dict:
    with urllib.request.urlopen(f"http://{host}{pfad}", timeout=timeout) as antwort:
        return json.load(antwort)


def uhrzeit(unix_sekunden: int) -> str:
    if not unix_sekunden:
        return "--:--"
    return datetime.fromtimestamp(unix_sekunden).strftime("%d.%m. %H:%M")


def zeige_aufnahmen(daten: dict) -> None:
    if not daten.get("ok"):
        print(f"SD-Karte nicht lesbar: {daten.get('status')}")
        return

    zaehlung = daten["counts"]
    print(
        f"{zaehlung['recordings']} Aufnahmen: "
        f"{zaehlung['notes']} Notizen, {zaehlung['todos']} Todos "
        f"({zaehlung['todos_open']} offen, {zaehlung['todos_completed']} erledigt), "
        f"{zaehlung['follow_ups']} zum Nachfassen"
    )
    print()

    eintraege = sorted(daten["recordings"], key=lambda e: e["created_unix_seconds"])
    for eintrag in eintraege:
        merkmale = []
        if eintrag["completed"]:
            merkmale.append("erledigt")
        if eintrag["follow_up"]:
            merkmale.append("nachfassen")
        if not eintrag["has_transcript"]:
            merkmale.append("OHNE TRANSKRIPT")
        anhang = f"  [{', '.join(merkmale)}]" if merkmale else ""
        print(
            f"{uhrzeit(eintrag['created_unix_seconds'])}  "
            f"{eintrag['tag']:5s}  {eintrag['duration_ms'] / 1000:4.1f}s{anhang}"
        )
        print(f"    {eintrag.get('transcript', '').strip() or '(kein Text)'}")


def zeige_zusammenfassung(name: str, eintrag: dict) -> None:
    print("=" * 68)
    if not eintrag["available"]:
        print(f"{name}: noch keine Zusammenfassung vorhanden")
        return

    herkunft = eintrag["metadata"]
    hinweise = []
    if herkunft["missing_transcript_item_count"]:
        hinweise.append(f"{herkunft['missing_transcript_item_count']} ohne Transkript uebersprungen")
    if herkunft["truncated"]:
        hinweise.append("Eingabe gekuerzt")
    if herkunft["chunked"]:
        hinweise.append("in Chunks gerechnet")

    print(
        f"{name} | erzeugt {uhrzeit(herkunft['generated_unix_seconds'])} | "
        f"{herkunft['transcript_item_count']} von {herkunft['source_item_count']} Aufnahmen | "
        f"Fenster {herkunft['window_days']} Tage"
        + (" | " + ", ".join(hinweise) if hinweise else "")
    )
    print()
    print(eintrag["text"].strip())


def main() -> int:
    host = sys.argv[1] if len(sys.argv) > 1 else STANDARD_HOST

    try:
        aufnahmen = hole(host, "/api/archive/recordings")
        zusammenfassungen = hole(host, "/api/archive/summaries")
    except OSError as fehler:
        print(f"Geraet unter {host} nicht erreichbar: {fehler}")
        return 1

    print("### AUFNAHMEN ###")
    print()
    zeige_aufnahmen(aufnahmen)
    print()
    print("### ZUSAMMENFASSUNGEN ###")
    print()
    zeige_zusammenfassung("NOTIZEN", zusammenfassungen["notes"])
    print()
    zeige_zusammenfassung("TODOS", zusammenfassungen["todos"])

    lauf = zusammenfassungen["request"]
    if lauf["in_flight"]:
        print()
        print(f"Laeuft gerade: {lauf['kind']} -- {lauf['status_message']}")
    elif lauf["error_code"]:
        print()
        print(f"Letzter Lauf gescheitert: {lauf['error_code']} -- {lauf['error_message']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
