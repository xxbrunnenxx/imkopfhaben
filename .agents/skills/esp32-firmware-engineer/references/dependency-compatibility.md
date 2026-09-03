# ESP-IDF-/Plugin-Kompatibilitätsnachweis (ESP-ADF, ESP-SR usw.)

Nutze diese Referenz vor Build/Debug/Flash, wenn externe ESP-Frameworks/-Plugins im Einsatz sind. Kompatibilität muss mit exakt-versionierten Belegen nachgewiesen werden.

## Kernregel

- Nicht fortfahren, bis konkrete Belege vorliegen, dass jedes Plugin/Framework mit der exakten ESP-IDF-Version und untereinander kompatibel ist (wenn sie zusammen genutzt werden).
- "Konkrete Belege" bedeutet exakte Versionen + eine überprüfbare Quelle (Matrix, Manifest, Release-Note, fixierte Kompatibilitätsdatei oder getestetes Upstream-Bundle).

## Warum das wichtig ist

- ESP-IDF, ESP-ADF und ESP-SR haben oft Versions-Randbedingungen, die nicht beliebig austauschbar sind.
- Individuelle Kompatibilität mit ESP-IDF reicht nicht aus, wenn mehrere Frameworks kombiniert werden.
- Ein Stack kann eine Prüfung bestehen (`ESP-SR` sagt `idf >= 5.0`) und trotzdem in der Praxis scheitern, wegen einer Lücke in der `ESP-ADF`-Matrix oder einer Cross-Stack-Inkompatibilität.

## Beleg-Quellen (bevorzugte Reihenfolge)

## 1. Projekt-fixierte Kompatibilitäts-Sperrdatei (am besten für wiederholte Builds)

- Eine eingecheckte Datei, die explizit fixiert:
  - ESP-IDF-Version/-Tag
  - Plugin-/Framework-Versionen/-Tags/-Commits (ESP-ADF, ESP-SR usw.)
  - Quelle des Kompatibilitätsnachweises (URL/Release/Matrix)
  - Datum/Notizen (optional)

`assets/templates/compatibility/` als Ausgangspunkt nutzen.

## 2. Offizielle Kompatibilitäts-Matrizen / Release-Notes

- ESP-ADF-README-Kompatibilitätsmatrix (Zeile für ADF-Release + Spalte für exakte IDF-Version)
- ESP-ADF-Release-Notes (exakt unterstützte IDF-Versionen)
- ESP-SKAINET-Release-/Bundle-Doku (bei Kombination von ESP-SR + Audio-Stack)

## 3. Komponenten-Manifeste / Abhängigkeits-Randbedingungen

- `idf_component.yml` `dependencies.idf`-Versionsbereich (z. B. ESP-SR)
- Verwaltete Komponenten-Sperrdateien und Versions-Fixierungen

Hinweis:
- Manifest-Bereiche sind nützliche Belege für Plugin-zu-IDF-Kompatibilität.
- Sie reichen meist nicht aus, um Plugin-A-zu-Plugin-B-Kompatibilität nachzuweisen.

## 4. Lokale, reproduzierbare Build-/Test-Belege

- Erfolgreicher Clean-Build mit exakten Versionen
- Smoke-Test oder Build eines Beispielprojekts
- Idealerweise mit dem Kompatibilitätsnachweis anschließend in einer Sperrdatei festgehalten

Build-Erfolg allein ist hilfreich, sollte aber Upstream-Versionsbelege nicht ersetzen, wenn bekannte Kompatibilitätsmatrizen existieren.

## Erforderliche Prüfungen für gängige Stacks

### ESP-IDF + ESP-ADF

- Exakte ESP-IDF-Version festhalten (Major.Minor.Patch oder Tag)
- Exakte ESP-ADF-Version/-Tag festhalten
- Verifizieren, dass die ESP-ADF-README-/Release-Matrix die gewählte ESP-IDF-Version explizit auflistet
- Verifizieren, dass die gewählte ADF-Zeile sie als unterstützt markiert

Falls die exakte IDF-Version nicht gelistet ist, gilt Kompatibilität als unbewiesen (keine Vorwärtskompatibilität annehmen).

### ESP-IDF + ESP-SR

- Exakte ESP-IDF-Version festhalten
- Exakte ESP-SR-Version/-Tag festhalten
- Verifizieren, dass der `esp-sr/idf_component.yml`-`dependencies.idf`-Bereich die gewählte ESP-IDF-Version einschließt
- Zusätzliche Ziel-Randbedingungen (Chip-Unterstützung, PSRAM-Empfehlungen usw.) aus der ESP-SR-Doku prüfen

### ESP-IDF + ESP-ADF + ESP-SR (Cross-Stack)

- Alle obigen individuellen Prüfungen bestehen
- Zusätzlich explizite Cross-Stack-Belege erforderlich:
  - Projekt-Kompatibilitäts-Sperrdatei, oder
  - ESP-SKAINET-Release-/Bundle-Dokumentation, oder
  - vom Nutzer bereitgestellte getestete Matrix mit exakten Versionen

Cross-Stack-Kompatibilität nicht aus zwei unabhängigen Prüfungen ableiten.

## Agenten-Ablauf

1. Genutzte Plugins/Frameworks und exakte Versionen erfassen.
2. Belege aus lokalen Manifesten/READMEs/Release-Docs sammeln.
3. Vor dem Build einen Beleg-Report (und optional eine Sperrdatei) schreiben.
4. Falls ein Punkt fehlt/unklar ist, stoppen und nach Versions-Änderungen oder freigegebenen Belegen fragen.

Referenz-Helfer:
- `scripts/check_plugin_compatibility.py` validiert gängige ESP-IDF-/ESP-ADF-/ESP-SR-Belege und schreibt `build/plugin-compatibility-evidence.txt`.
- `ESP_REQUIRED_PLUGINS=esp-adf,esp-sr` setzen, um Prüfungen zu erzwingen, wenn die Auto-Erkennung unsicher ist.
- `ESP_STACK_COMPAT_EVIDENCE=...` setzen oder eine Projekt-Kompatibilitäts-Sperrdatei ergänzen, um Cross-Stack-Nachweispflichten zu erfüllen.

## Beispiel: Wie "unbewiesen" aussieht

- Die ESP-ADF-Matrix unterstützt bis IDF `v5.3`, aber das Projekt läuft auf IDF `v5.5`.
- Das ESP-SR-Manifest sagt `idf >= 5.0` und besteht die Prüfung.
- Ergebnis: der Stack gilt weiterhin als unbewiesen, weil der ADF-zu-IDF-Beleg für `v5.5` fehlt.

## Review-Checkliste

- Jedes genutzte Framework hat eine exakte Version/einen Tag/einen Commit festgehalten.
- Jedes Framework hat einen expliziten Kompatibilitätsnachweis gegen die exakte ESP-IDF-Version.
- Cross-Stack-Belege existieren, wenn mehrere Frameworks zusammenwirken.
- Der Nachweis wird vor dem Build in einem Report oder einer Sperrdatei festgehalten.
- Keine "wahrscheinlich kompatibel"-Annahmen bleiben bestehen.
