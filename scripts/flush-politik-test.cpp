// Belegt die Flush-Politik aus ssd1677_driver.cpp und DisplayTask am Host.
// Handgriff zu den entsprechenden Zeilen in docs/PRUEFUNG.md.
//
// Nachgebaut sind die drei Groessen, die zusammen entscheiden, ob es blitzt:
//   - partial_refresh_count_ im Panel
//   - die weiche Schwelle (Flush faellig) und die harte Grenze (Flush erzwungen)
//   - das flush_due-Flag in DisplayTask, das den begrenzten Queue-Wait scharf macht
#include <cstdio>

constexpr int kMaxPartialRefreshesBeforeFlush = 8;
constexpr int kMaxPartialRefreshesHardCap = 60;

struct Panel {
    int partial_refresh_count_ = 0;
    bool base_image_initialized_ = true;
    bool active_ = true;

    bool CanPartialRefresh(int max) const
    {
        return active_ && base_image_initialized_ && partial_refresh_count_ < max;
    }
    bool DeferredFlushPending() const
    {
        return active_ && base_image_initialized_ &&
               partial_refresh_count_ >= kMaxPartialRefreshesBeforeFlush;
    }
    // Rueckgabe: true = es hat geblitzt (Voll-Refresh gefahren).
    bool Partial()
    {
        if (!CanPartialRefresh(kMaxPartialRefreshesHardCap)) {
            partial_refresh_count_ = 0;  // RefreshFullBase setzt zurueck
            return true;
        }
        ++partial_refresh_count_;
        return false;
    }
    void ScreenChange() { partial_refresh_count_ = 0; }  // Full/Fast base setzt zurueck
    void Sleep() { partial_refresh_count_ = 0; base_image_initialized_ = false; active_ = false; }
    void Wake() { base_image_initialized_ = true; active_ = true; }
};

// Nachbau der DisplayTask-Schleife inklusive flush_due-Flag.
struct Task {
    Panel panel;
    bool flush_due = false;
    bool display_sleeping = false;

    bool DeferredFlushDue() const { return !display_sleeping && panel.DeferredFlushPending(); }
    // Ein Kommando, das einen Partial faehrt.
    bool PartialCommand() { const bool blitz = panel.Partial(); flush_due = DeferredFlushDue(); return blitz; }
    bool ScreenChangeCommand() { panel.ScreenChange(); flush_due = DeferredFlushDue(); return true; }
    // Der Timeout-Zweig. Rueckgabe: true = Flush wurde gefahren.
    bool IdleTimeout()
    {
        if (!DeferredFlushDue()) { flush_due = false; return false; }
        panel.partial_refresh_count_ = 0;
        flush_due = DeferredFlushDue();
        return true;
    }
    // Blockiert die Aufgabe unbegrenzt? Dann kann kein Flush mehr nachkommen.
    bool BlocksForever() const { return !flush_due; }
};

static int g_rc = 0;
static void Check(const char* was, bool ist, bool erwartet)
{
    printf("%-58s %-5s (erwartet %s)%s\n", was, ist ? "ja" : "nein", erwartet ? "ja" : "nein",
           ist == erwartet ? "" : "   <-- FEHLSCHLAG");
    if (ist != erwartet) g_rc = 1;
}

int main()
{
    printf("== Panel-Zaehllogik ==\n");
    {   // Dauerscrollen ohne Pause: erzwungener Blitz erst an der harten Grenze.
        Panel p; int first = -1;
        for (int i = 1; i <= 100; i++) if (p.Partial() && first < 0) first = i;
        printf("%-58s %-5d (erwartet %d)%s\n", "Dauerscrollen: erster erzwungener Blitz bei Partial",
               first, kMaxPartialRefreshesHardCap + 1,
               first == kMaxPartialRefreshesHardCap + 1 ? "" : "   <-- FEHLSCHLAG");
        if (first != kMaxPartialRefreshesHardCap + 1) g_rc = 1;
    }
    {   // Der gemeldete Fall: acht Partials am Stueck. Vorher blitzte es beim neunten.
        Panel p; bool blitz = false;
        for (int i = 0; i < 8; i++) blitz |= p.Partial();
        Check("8 Partials am Stueck: Blitz mitten in der Bewegung", blitz, false);
        Check("8 Partials am Stueck: Flush faellig", p.DeferredFlushPending(), true);
    }
    {   Panel p; for (int i = 0; i < 7; i++) p.Partial();
        Check("7 Partials: Flush noch nicht faellig", p.DeferredFlushPending(), false);
    }

    printf("\n== DisplayTask-Zustandsmaschine ==\n");
    {   Task t;
        Check("Frisch gestartet: blockiert unbegrenzt (kein Aufwachen)", t.BlocksForever(), true);
    }
    {   Task t; for (int i = 0; i < 3; i++) t.PartialCommand();
        Check("Nach 3 Partials: blockiert weiter unbegrenzt", t.BlocksForever(), true);
        Check("Leerlauf nach 3: Flush gefahren", t.IdleTimeout(), false);
    }
    {   Task t; for (int i = 0; i < 8; i++) t.PartialCommand();
        Check("Nach 8 Partials: begrenzter Wait scharf", !t.BlocksForever(), true);
        Check("Leerlauf nach 8: Flush gefahren", t.IdleTimeout(), true);
        Check("Danach: Zaehler zurueck, blockiert wieder unbegrenzt", t.BlocksForever(), true);
    }
    {   // Der Fall aus dem echten Mitschnitt: Nutzer wechselt die Seite, bevor es ruhig wird.
        Task t; for (int i = 0; i < 8; i++) t.PartialCommand();
        t.ScreenChangeCommand();
        Check("Screenwechsel nach 8 Partials: Flush entfaellt", t.BlocksForever(), true);
        Check("Danach Leerlauf: kein nachtraeglicher Blitz", t.IdleTimeout(), false);
    }
    {   // Displayschlaf darf keinen Flush auf ein schlafendes Panel fahren.
        Task t; for (int i = 0; i < 8; i++) t.PartialCommand();
        t.display_sleeping = true; t.panel.Sleep();
        Check("Displayschlaf nach 8 Partials: kein Flush im Schlaf", t.IdleTimeout(), false);
        t.display_sleeping = false; t.panel.Wake();
        Check("Nach dem Aufwachen: kein Altlast-Flush (Zaehler war zurueck)", t.IdleTimeout(), false);
    }
    {   // Der Sleeping-Zweig in DisplayTask setzt flush_due NICHT neu, sondern macht
        // nur `continue`. Dieser Fall belegt, dass sich das nach einem einzigen
        // Timeout selbst heilt und das Geraet danach wieder unbegrenzt blockiert.
        Task t; for (int i = 0; i < 8; i++) t.PartialCommand();
        Check("Vor dem Schlaf: begrenzter Wait scharf", !t.BlocksForever(), true);
        t.display_sleeping = true; t.panel.Sleep();
        Check("Kommando im Schlaf laesst flush_due stehen", !t.BlocksForever(), true);
        Check("Erster Timeout im Schlaf: kein Flush", t.IdleTimeout(), false);
        Check("...und danach wieder unbegrenzt blockiert", t.BlocksForever(), true);
    }
    {   // Scrollen mit Pausen: nie ein Blitz mitten in der Bewegung, ueber viele Runden.
        Task t; bool blitz = false; int fluesche = 0;
        for (int runde = 0; runde < 20; runde++) {
            for (int i = 0; i < 8; i++) blitz |= t.PartialCommand();
            if (t.IdleTimeout()) fluesche++;
        }
        Check("20 Runden a 8 Partials mit Pause: Blitz in der Bewegung", blitz, false);
        printf("%-58s %-5d (erwartet 20)%s\n", "20 Runden: Fluesche im Leerlauf", fluesche,
               fluesche == 20 ? "" : "   <-- FEHLSCHLAG");
        if (fluesche != 20) g_rc = 1;
    }
    {   // Pathologischer Fall: nie Ruhe. Die harte Grenze muss trotzdem greifen.
        Task t; int blitze = 0;
        for (int i = 0; i < 200; i++) if (t.PartialCommand()) blitze++;
        printf("%-58s %-5d (erwartet 3)%s\n", "200 Partials ohne jede Pause: erzwungene Blitze",
               blitze, blitze == 3 ? "" : "   <-- FEHLSCHLAG");
        if (blitze != 3) g_rc = 1;
    }

    printf(g_rc ? "\nFEHLSCHLAG\n" : "\nALLE GRUEN\n");
    return g_rc;
}
