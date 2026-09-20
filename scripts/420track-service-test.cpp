// Reset-Test: treibt die ECHTE joint_tracker_service.cpp ueber einen
// Tageswechsel. Die Uhr wird ueber __wrap_time gesteuert (Linker --wrap=time).
#include <cstdio>
#include <ctime>

#include "joint_tracker_service.h"

// Steuerbare Uhr: der Service ruft time(nullptr); wir liefern g_now.
extern "C" time_t __wrap_time(time_t* t) {
    extern time_t g_now;
    if (t) *t = g_now;
    return g_now;
}
time_t g_now = 0;

// Hilfen: ein Datum (Ortszeit) in einen time_t umrechnen.
static time_t at_local(int y, int mon, int d, int h, int mi) {
    struct tm tmv = {};
    tmv.tm_year = y - 1900; tmv.tm_mon = mon - 1; tmv.tm_mday = d;
    tmv.tm_hour = h; tmv.tm_min = mi; tmv.tm_isdst = -1;
    return mktime(&tmv);
}

static int fails = 0;
#define CHECK(cond, msg) do { \
    if (cond) printf("  OK   %s\n", msg); \
    else { printf("  FAIL %s\n", msg); ++fails; } \
} while (0)

int main() {
    using namespace joint_tracker_service;

    printf("== 420-Track Mitternachts-Reset (echter Service) ==\n");

    // Tag 1: 20.09.2026, kurz vor Mitternacht. Erst laden (frisches NVS).
    g_now = at_local(2026, 9, 20, 23, 30);
    Load();
    CHECK(GetTodayCount() == 0, "Start: heutiger Zaehler 0");

    // Drei Joints an Tag 1.
    Increment(); Increment(); Increment();
    CHECK(GetTodayCount() == 3, "Tag 1: nach 3x Increment -> 3");
    printf("  Tag 1 Stand: %d, Protokoll %zu Tage\n", GetTodayCount(), GetHistory().size());

    // --- Mitternacht ueberschreiten: 21.09.2026, 00:05 ---
    g_now = at_local(2026, 9, 21, 0, 5);
    bool changed = RolloverIfNeeded();
    CHECK(changed, "Tageswechsel wurde erkannt (RolloverIfNeeded == true)");
    CHECK(GetTodayCount() == 0, "Tag 2: Zaehler steht nach Mitternacht auf 0");

    auto hist = GetHistory();
    // Protokoll enthaelt Tag 1 (20260920) mit 3, plus heutigen Tag (0).
    bool tag1_ok = false, heute_ok = false;
    for (auto& e : hist) {
        if (e.day == 20260920 && e.count == 3) tag1_ok = true;
        if (e.day == 20260921 && e.count == 0) heute_ok = true;
        printf("  Protokoll: Tag %d = %d\n", e.day, e.count);
    }
    CHECK(tag1_ok, "Gestriger Stand (20.09. = 3) ist ins Protokoll gewandert");
    CHECK(heute_ok, "Heutiger Tag (21.09. = 0) steht im Protokoll");

    // Tag 2 zaehlt frisch weiter.
    Increment();
    CHECK(GetTodayCount() == 1, "Tag 2: Increment zaehlt frisch ab 0 -> 1");

    // --- Zweiter Tageswechsel ohne Tastendruck (nur Getter) ---
    g_now = at_local(2026, 9, 22, 8, 0);
    int c = GetTodayCount();  // loest Rollover selbst aus
    CHECK(c == 0, "Tag 3: Getter allein loest Reset aus (0), auch ohne Increment");
    // Protokoll haelt jetzt 20./21. + heute.
    int found20 = 0, found21 = 0;
    for (auto& e : GetHistory()) {
        if (e.day == 20260920 && e.count == 3) found20 = 1;
        if (e.day == 20260921 && e.count == 1) found21 = 1;
    }
    CHECK(found20 && found21, "Protokoll behaelt 20.09.=3 und 21.09.=1 lueckenlos");

    // --- Zaehl-Logik: Increment/Decrement, nicht unter 0, ueber Ziel erlaubt ---
    // Tag 3 frisch bei 0.
    Increment(); Increment();               // 2
    CHECK(GetTodayCount() == 2, "Increment 2x -> 2");
    Decrement();                            // 1
    CHECK(GetTodayCount() == 1, "Decrement -> 1");
    Decrement(); Decrement();               // nicht unter 0
    CHECK(GetTodayCount() == 0, "Decrement unter 0 bleibt bei 0 (kein Negativ)");
    for (int i = 0; i < 6; ++i) Increment(); // ueber Ziel 4
    CHECK(GetTodayCount() == 6, "Ueber Ziel erlaubt (6 > goal 4, nicht gedeckelt)");
    CHECK(GetDailyGoal() == 4, "Tagesrichtwert ist 4");

    printf(fails ? "\nERGEBNIS: %d FEHLER\n" : "\nERGEBNIS: alles gruen\n", fails);
    return fails ? 1 : 0;
}
