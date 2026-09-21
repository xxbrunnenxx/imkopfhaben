// Monkey-/Property-Test der Todos-Navigationslogik (Host-Build auf dem Pi).
//
// Er bombardiert den ECHTEN TodosPageCoordinator + die ECHTEN
// todos_page_interactions mit zufaelligen Tastenfolgen und prueft nach jedem
// Schritt harte Invarianten, die nie brechen duerfen. Ziel ist die KLASSE
// Fehler, die der Besitzer erlebt hat: gelockte Auswahl, aus der man nicht
// mehr herauskommt, sowie in sich widerspruechliche Zustaende und Abstuerze.
//
// Nicht getestet: Darstellung, Lesbarkeit, Haptik -- das kann ein Host-Test
// ohne E-Paper und Tasten nicht sehen. Das bleibt die Sichtpruefung am Geraet.
//
// Die "Tasten" bilden die realen Wege aus page_input_runtime.cpp nach:
//   UP / DOWN            -> MoveFocus(-1) / MoveFocus(+1)
//   OK (Primary)         -> HandlePrimaryActivate (Chip betreten / Modal / Footer)
//   BACK (DoubleClick)   -> ExitActiveControl (Item-Liste verlassen)
//   ENTER_TODOS          -> frisches Betreten der Seite  (Show)
//   REFRESH_ACTIVE       -> Archiv-Aenderung waehrend Seite offen (RefreshFromArchive)
//   MUTATE               -> Todo abhaken/loeschen/Follow-up (veraendert das Archiv)

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "todos_page_coordinator.h"
#include "todos_page_interactions.h"
#include "recording_archive_service.h"

using recording_archive_service::RecordingEntry;
using recording_archive_service::RecordingTag;

namespace {

// ---- Ein Test-Archiv, das wir zwischen Laeufen veraendern koennen ----------
struct FakeArchive {
    std::vector<RecordingEntry> entries;
    int next_id = 1;

    void AddTask(const std::string& date, int64_t unix_seconds, bool completed) {
        AddEntry(RecordingTag::kTask, date, unix_seconds, "Aufgabe", completed);
    }

    // Allgemeiner Eintrag mit frei waehlbarem Tag/Text -- fuer das grosse
    // realistische Startset (Notizen, Aufgaben, Ideen gemischt).
    void AddEntry(RecordingTag tag, const std::string& date, int64_t unix_seconds,
                  const std::string& text, bool completed) {
        RecordingEntry e = {};
        e.recording_id = "rec_" + std::to_string(next_id++);
        e.recording_path = "/sd/" + e.recording_id + ".wav";
        e.transcript_text = text + " " + e.recording_id;
        e.modified_unix_seconds = unix_seconds;
        e.metadata.recording_id = e.recording_id;
        e.metadata.created_unix_seconds = unix_seconds;
        e.metadata.created_local_date = date;
        e.metadata.time_valid = true;
        e.metadata.has_transcript = true;
        e.metadata.duration_ms = 3000 + (next_id % 20) * 900;
        e.metadata.completed = completed;
        e.metadata.tag = tag;
        entries.push_back(std::move(e));
    }
};

// ---- Realistisches Startset -------------------------------------------------
// Kurze, echte Sprachnotiz-Texte, wie sie das Geraet taeglich sammelt:
// gemischt aus kurzen Kommandos/Aufgaben, Notizen und Ideen, verteilt ueber
// viele Kalendertage. Der Todos-Coordinator filtert selbst auf kTask -- das
// Set testet damit auch den Filter unter Last (Notizen/Ideen als Ballast).
void SeedRealistic(FakeArchive& a, int target, std::mt19937_64& rng) {
    static const char* aufgaben[] = {
        "Muell rausbringen", "Zahnarzt anrufen", "Rechnung bezahlen",
        "Milch kaufen", "Paket abholen", "Oma zurueckrufen", "Auto tanken",
        "Wasser fuer die Pflanzen", "Termin bestaetigen", "Akku laden",
        "Buch zurueckgeben", "Fenster putzen", "Steuer sortieren",
        "Kraken updaten", "Backup pruefen", "SD-Karte formatieren",
    };
    static const char* notizen[] = {
        "Idee war ganz gut heute", "der Regen war schoen", "Kaffee war zu stark",
        "Gespraech mit Tom", "Podcast-Folge merken", "Zitat aus dem Buch",
        "Traum von gestern", "Gedanke beim Laufen", "Wetter dreht sich",
        "Rezept ausprobiert", "Preis vergleichen", "Adresse notiert",
    };
    static const char* ideen[] = {
        "App fuer Vogelstimmen", "Regal selber bauen", "Reise nach Norden",
        "Podcast starten", "Garten umgraben", "kleines Spiel programmieren",
        "Brief schreiben", "Fotobuch machen", "Sprache lernen",
    };

    // ~60 Tage rueckwaerts ab heute, ungleich verteilt (manche Tage voll,
    // manche leer) -- so entstehen viele Gruppen unterschiedlicher Groesse.
    std::uniform_int_distribution<int> tag_zurueck(0, 59);
    std::uniform_int_distribution<int> art(0, 99);
    std::uniform_int_distribution<int> ai(0, 15);
    std::uniform_int_distribution<int> ni(0, 11);
    std::uniform_int_distribution<int> ii(0, 8);
    const int64_t heute = 1'700'000'000;  // fixer Bezugspunkt

    for (int i = 0; i < target; ++i) {
        const int d = tag_zurueck(rng);
        // Datum als 2026-07-DD..2026-09-DD grob abbilden (Text reicht dem Test,
        // die Gruppierung nutzt created_local_date als String-Schluessel).
        char buf[16];
        std::snprintf(buf, sizeof(buf), "2026-%02d-%02d", 7 + (d / 28), 1 + (d % 28));
        const int64_t ts = heute - static_cast<int64_t>(d) * 86400 - i;
        const int r = art(rng);
        if (r < 55) {  // 55% Aufgaben (das ist, was die Todos-Seite zeigt)
            const bool done = (rng() % 3) == 0;  // ~1/3 erledigt
            a.AddEntry(RecordingTag::kTask, buf, ts, aufgaben[ai(rng)], done);
        } else if (r < 85) {  // 30% Notizen (Ballast fuer den Filter)
            a.AddEntry(RecordingTag::kNote, buf, ts, notizen[ni(rng)], false);
        } else {  // 15% Ideen
            a.AddEntry(RecordingTag::kIdea, buf, ts, ideen[ii(rng)], false);
        }
    }
}

// ---- Invarianten-Pruefung: gibt bei Bruch eine Meldung zurueck -------------
// Wir pruefen ausschliesslich ueber die oeffentliche API + BuildState(), also
// genau das, was auch die Anzeige und die Eingabe-Schicht sehen.
std::string CheckInvariants(const TodosPageCoordinator& c) {
    const epaper_ui::TodosPageState st = c.BuildState();
    const int group_count = c.TimelineGroupCount();

    // (1) Es gibt immer mindestens eine Gruppe (leere Seite -> "Today"-Platzhalter).
    if (group_count < 1) return "keine Gruppe vorhanden";

    // (2) Der Navigations-Fokusindex ist gueltig.
    if (st.navigation_focus_index < 0) return "navigation_focus_index < 0";

    // (3) Wenn eine Item-Liste aktiv ist, muss sie eine gueltige Gruppe treffen
    //     und einen gueltigen Auswahlindex haben (der Kern des gemeldeten Bugs).
    const auto& tl = st.timeline;
    if (tl.active_group_index >= 0) {
        if (tl.active_group_index >= group_count)
            return "active_group_index zeigt auf nicht existierende Gruppe";
        const int items = static_cast<int>(tl.groups[tl.active_group_index].items.size());
        if (items <= 0)
            return "Item-Liste aktiv, aber Gruppe hat keine Eintraege";
        if (tl.selected_item_index < 0 || tl.selected_item_index >= items)
            return "selected_item_index ausserhalb der Liste";
    } else {
        // Nicht aktiv -> es darf kein Item als ausgewaehlt gemeldet werden.
        if (tl.selected_item_index != -1)
            return "selected_item_index gesetzt, obwohl keine Liste aktiv";
    }

    // (4) sichtbarer/fokussierter Gruppenindex bleiben im Rahmen.
    if (tl.visible_group_index < -1 || tl.visible_group_index >= group_count)
        return "visible_group_index ausserhalb";
    if (tl.focused_group_index < -1 || tl.focused_group_index >= group_count)
        return "focused_group_index ausserhalb";

    return {};
}

// Bildet SyncFromArchive(false) nach: frisches Betreten der Seite.
void EnterPage(TodosPageCoordinator& c, const FakeArchive& a) { c.Show(a.entries); }
// Bildet SyncFromArchive(true) nach: Refresh, waehrend die Seite offen ist.
void RefreshActive(TodosPageCoordinator& c, const FakeArchive& a) { c.RefreshFromArchive(a.entries); }

// Ein OK-Druck, exakt nach todos_page_interactions::HandlePrimaryActivate.
void PressOk(TodosPageCoordinator& c) {
    const todos_page_interactions::ActivateResult r =
        todos_page_interactions::HandlePrimaryActivate(c);
    // kOpenItemActions / kShowHome etc. wuerden in der App die Seite wechseln
    // bzw. ein Modal oeffnen. Fuer die reine Coordinator-Logik ist der
    // Zustandsuebergang (Chip betreten) bereits in HandlePrimaryActivate
    // passiert; die Intents brauchen hier keine Weiterverarbeitung.
    (void)r;
}

}  // namespace

int main(int argc, char** argv) {
    // Reproduzierbar: Seed als Argument, sonst fest. Schrittzahl als 2. Arg,
    // Groesse des Startsets als 3. Arg (0 = kleines festes Set wie v1).
    uint64_t seed = (argc > 1) ? std::stoull(argv[1]) : 20260921ULL;
    long steps = (argc > 2) ? std::stol(argv[2]) : 2'000'000L;
    int seed_count = (argc > 3) ? std::stoi(argv[3]) : 0;

    std::mt19937_64 rng(seed);

    FakeArchive archive;
    if (seed_count > 0) {
        // Grosses, realistisches Startset (v2): Aufgaben/Notizen/Ideen ueber
        // viele Tage gemischt.
        SeedRealistic(archive, seed_count, rng);
    } else {
        // Kleines festes Set (v1): mehrere Tage, mehrere Todos pro Tag.
        archive.AddTask("2026-09-21", 1'000'100, false);
        archive.AddTask("2026-09-21", 1'000'050, true);
        archive.AddTask("2026-09-20", 1'000'000, false);
        archive.AddTask("2026-09-19",   999'900, false);
        archive.AddTask("2026-09-19",   999'800, true);
    }

    // Kennzahlen des Startsets ausgeben (Tags + Tage), damit der Bericht
    // belegte Zahlen hat statt behaupteter.
    {
        long t = 0, n = 0, ie = 0;
        std::vector<std::string> tage;
        for (const auto& e : archive.entries) {
            switch (e.metadata.tag) {
                case RecordingTag::kTask: ++t; break;
                case RecordingTag::kNote: ++n; break;
                case RecordingTag::kIdea: ++ie; break;
            }
        }
        // grobe Tageszahl
        std::vector<std::string> d;
        for (const auto& e : archive.entries) d.push_back(e.metadata.created_local_date);
        std::sort(d.begin(), d.end());
        d.erase(std::unique(d.begin(), d.end()), d.end());
        std::printf("startset: %zu eintraege  (aufgaben=%ld notizen=%ld ideen=%ld)  ueber %zu tage\n",
                    archive.entries.size(), t, n, ie, d.size());
    }

    TodosPageCoordinator coord;
    EnterPage(coord, archive);

    // Zaehler fuers Schlusswort.
    long ok_presses = 0, moves = 0, backs = 0, enters = 0, refreshes = 0, mutates = 0;
    long empty_entries = 0;  // wie oft die Seite ganz leer war

    std::uniform_int_distribution<int> action(0, 99);

    std::string broken;
    long step = 0;
    for (; step < steps; ++step) {
        const int a = action(rng);
        if (a < 34) {                    // 34% Fokus bewegen
            const int delta = (a % 2 == 0) ? 1 : -1;
            todos_page_interactions::HandleMoveFocus(coord, delta);
            ++moves;
        } else if (a < 58) {             // 24% OK
            PressOk(coord);
            ++ok_presses;
        } else if (a < 74) {             // 16% BACK (Item-Liste verlassen)
            coord.ExitItemList();
            ++backs;
        } else if (a < 84) {             // 10% frisches Betreten der Seite
            EnterPage(coord, archive);
            ++enters;
        } else if (a < 92) {             // 8% Refresh waehrend Seite offen
            RefreshActive(coord, archive);
            ++refreshes;
        } else {                         // 8% Archiv veraendern (abhaken/loeschen/hinzu)
            ++mutates;
            // Gewichtet, damit der GEFUELLTE Zustand dominiert -- genau dort
            // kann sich die Auswahl verklemmen. "Alles loeschen" bleibt selten
            // (der Leer-Platzhalter ist trotzdem oft genug abgedeckt), Nachfuellen
            // ist kraeftig, damit die Liste nicht ausblutet.
            std::uniform_int_distribution<int> pick(0, 19);
            const int kind = pick(rng);
            if (kind <= 4 && !archive.entries.empty()) {
                // 25%: Ein zufaelliges Todo loeschen.
                std::uniform_int_distribution<size_t> idx(0, archive.entries.size() - 1);
                archive.entries.erase(archive.entries.begin() + idx(rng));
            } else if (kind == 5) {
                // 5%: Alle loeschen -> leere Seite (Platzhalter "Today").
                archive.entries.clear();
            } else if (kind <= 12) {
                // 35%: Neue(s) Todo(s) an wechselnden Tagen (kraeftig nachfuellen).
                static const char* tage[] = {"2026-09-21", "2026-09-20", "2026-09-19"};
                std::uniform_int_distribution<int> td(0, 2);
                const bool done = (rng() & 1u) != 0;
                archive.AddTask(tage[td(rng)], 1'000'000 + archive.next_id, done);
            } else if (!archive.entries.empty()) {
                // 35%: Completion eines Eintrags kippen.
                std::uniform_int_distribution<size_t> idx(0, archive.entries.size() - 1);
                auto& m = archive.entries[idx(rng)].metadata;
                m.completed = !m.completed;
            }
            // Nach einer Archivaenderung folgt in der App IMMER ein
            // SyncFromArchive(true) auf der aktiven Seite.
            RefreshActive(coord, archive);
        }

        if (coord.HasOnlyEmptyGroup()) ++empty_entries;

        broken = CheckInvariants(coord);
        if (!broken.empty()) break;
    }

    std::printf("seed=%llu  schritte=%ld/%ld\n",
                (unsigned long long)seed, step, steps);
    std::printf("  moves=%ld ok=%ld back=%ld enter=%ld refresh=%ld mutate=%ld  (leer gesehen: %ldx)\n",
                moves, ok_presses, backs, enters, refreshes, mutates, empty_entries);

    if (!broken.empty()) {
        std::printf("INVARIANTE GEBROCHEN bei Schritt %ld: %s\n", step, broken.c_str());
        return 1;
    }
    std::printf("OK -- keine Invariante gebrochen, kein Absturz, keine Sackgasse.\n");
    return 0;
}
