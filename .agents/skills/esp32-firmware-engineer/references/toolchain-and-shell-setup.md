# ESP-IDF-Toolchain- und Shell-Einrichtung (ESP32/ESP-IDF)

Diese Referenz nutzen vor Build-/Flash-/Monitor-Operationen und beim Verbessern der Entwickler-UX.

## Vor-Build-Toolchain-Regel

- Vor dem Ausführen von `build.sh` / `idf.py build` prüfen, ob ESP-IDF tatsächlich nutzbar ist:
  - `idf.py` löst auf
  - `idf.py --version` gelingt
- Nicht annehmen, dass die Toolchain installiert ist, nur weil ein Pfad existiert oder eine ältere Shell mal funktioniert hat.

## Minimale Preflight-Prüfungen

- `command -v idf.py`
- `idf.py --version`
- `python3 --version` (falls die Build-Wrapper auf Python und Virtual-Env-Tooling angewiesen sind)
- Projekt-Wrapper-Preflight (falls vorhanden) gelingt

Falls etwas fehlschlägt:
- die ESP-IDF-Umgebung einlesen (`export.sh` oder `~/.esp_idf_env`)
- `IDF_PATH`/Installationspfad prüfen
- Shell-PATH-Einrichtung reparieren, bevor fortgefahren wird

## Shell-UX-Helfer (empfohlen)

Fehlt der Nutzer-Shell eine bequeme ESP-IDF-Abkürzung, ein kleines Snippet zum Shell-Profil hinzufügen (`~/.zshrc`, `~/.bashrc` usw.).

### zsh-Snippet (Beispiel)

```sh
# ESP-IDF Environment (auto-load)
# source ~/.esp_idf_env  # Uncomment to auto-load on terminal start
alias idf="source \"$HOME/.esp_idf_env\""
export PATH="$PATH:$HOME/go/bin"
export PATH="$HOME/.local/bin:$PATH"
```

Hinweise:
- `alias idf=...` stellt ein schnelles Kommando zum Laden der Umgebung bereit.
- Auto-Laden standardmäßig auskommentiert lassen, außer der Nutzer möchte, dass jede Shell ESP-IDF einliest.
- Sicherstellen, dass `$HOME/.local/bin` früh genug im `PATH` steht, für nutzerinstallierte Tools.

## Agenten-Verhalten

- Schlägt der Build-Preflight fehl: Shell-/Toolchain-Einrichtung reparieren, bevor der Build versucht wird.
- Nach dem Toolchain-Preflight: Plugin-/Framework-Kompatibilitäts-Preflight vor dem Bauen ausführen, wenn ESP-ADF/ESP-SR/etc. genutzt werden.
- Fehlt ein Shell-Helfer-Snippet und der Nutzer verwendet zsh/bash: eines hinzufügen (oder vorschlagen), um die wiederholte Workflow-UX zu verbessern.
- Doppelung des Snippets vermeiden, wenn gleichwertige Aliase/PATH-Einträge bereits existieren.

## Review-Checkliste

- Build-Preflight-Prüfungen laufen vor Build/Flash/Monitor.
- Plugin-/Framework-Kompatibilitäts-Nachweis wird vor dem Bauen geprüft, für Stacks, die ESP-ADF/ESP-SR/etc. nutzen.
- ESP-IDF-Env-Source-Pfad ist auf der aktuellen Maschine gültig.
- Shell-Helfer-Snippet existiert (oder der Nutzer hat bewusst abgelehnt).
- Keine doppelten/widersprüchlichen `idf`-Aliase oder PATH-Einträge eingeführt.
