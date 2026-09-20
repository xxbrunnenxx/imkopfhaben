// Host-Harness: rendert die ECHTE 420-Track-Karte (joint_tracker_card.cpp)
// in einen 1bpp-Framebuffer wie auf dem Geraet (raw 800x480, portrait 480x800)
// und schreibt sie als PGM. Zweck: die Punktdarstellung (gefuellt/offen/ueber
// Limit mit hellem Kern) sichtbar und messbar belegen, ohne den Schirm.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "epaper_ui/joint_tracker_card.h"

using namespace epaper_ui;

static constexpr int kRawW = 800;
static constexpr int kRawH = 480;
static constexpr int kPortW = 480;   // = raw_height
static constexpr int kPortH = 800;   // = raw_width

// Portrait-Pixel lesen (invertiert zur Zeichenlogik in render_utils):
// black-Bit = 0 im Framebuffer. Rueckgabe: true = schwarz (gesetzt).
static bool PortraitBlack(const std::vector<uint8_t>& fb, int x, int y)
{
    const int raw_x = y;
    const int raw_y = kRawH - 1 - x;
    const int bytes_per_row = kRawW / 8;
    const size_t index = static_cast<size_t>(raw_y) * bytes_per_row + raw_x / 8;
    const uint8_t mask = static_cast<uint8_t>(0x80U >> (raw_x & 7));
    return (fb[index] & mask) == 0;  // 0 = schwarz
}

static void analyse(int count, int goal, const char* pgm)
{
    std::vector<uint8_t> fb(static_cast<size_t>(kRawW) * kRawH / 8, 0xFF);  // alles weiss

    JointTrackerCardState st;
    st.count = count;
    st.goal = goal;
    st.focused = true;

    JointTrackerCardStyle style;
    style.width = kPortW - 32;  // wie Dashboard: page_width mit Margin 16

    const int ox = 16, oy = 40;
    const UiRect b = JointTrackerCardBounds(ox, oy, st, style);
    DrawJointTrackerCard(fb.data(), kRawW, kRawH, kPortW, kPortH, ox, oy, st, style);

    // Punkte an ihren EXAKT bekannten Positionen aus dem Kartencode messen,
    // statt blind zu segmentieren (der graue Dither-Hintergrund verfaelscht
    // jede Schwellwert-Segmentierung). Geometrie wie in DrawJointTrackerCard:
    //   content_x = ox + padding;  dots_y = content_y + LineHeight(label)+gap
    //   Punkt i bei dot_x = content_x + i*(dot+dot_gap), Mitte +dot/2
    const int dot = style.dot_diameter;      // 14
    const int gap = style.dot_gap;           // 8
    const int pad = style.padding;           // 8
    const int content_x = ox + pad;
    const int total = (st.count > st.goal ? st.count : st.goal);

    printf("--- count=%d, goal=%d ---\n", count, goal);
    printf("Karte bounds: x=%d y=%d w=%d h=%d\n", b.x, b.y, b.width, b.height);
    printf("Erwartet: %d Punkte gesamt (goal=%d, count=%d)\n", total, st.goal, st.count);

    // dots_y bestimmen wir robust: das schwarze Band im unteren Kartendrittel.
    // Wir suchen die Zeile mit dem laengsten horizontalen Schwarzanteil an den
    // Punktspalten.
    int dots_y = b.y + b.height - pad - dot;  // aus der Geometrie

    int solid = 0, ring = 0, outline = 0, leer = 0;
    for (int i = 0; i < total; ++i) {
        const int dx = content_x + i * (dot + gap);
        const int cx = dx + dot / 2;
        const int cy = dots_y + dot / 2;
        // Rand der Scheibe (nahe Mitte-oben) vs. echte Mitte:
        const bool center_black = PortraitBlack(fb, cx, cy);
        // Ringtest: bei einem Umriss ist die Mitte NICHT schwarz, aber der
        // Rand (cx, dots_y+1) schon. Bei gefuellt-mit-Kern ist die Mitte hell
        // und ein Kranz darum schwarz. Wir zaehlen schwarze Pixel auf einem
        // kleinen Kreuz durch die Mitte und am Rand.
        int rand_black = 0;
        for (int t = 0; t < dot; ++t)
            if (PortraitBlack(fb, dx + t, dots_y + dot/2)) ++rand_black;  // Horizontallinie
        // Kernpixel (3x3 um die Mitte)
        int kern_black = 0, kern_n = 0;
        for (int yy=-1; yy<=1; ++yy) for (int xx=-1; xx<=1; ++xx) {
            ++kern_n; if (PortraitBlack(fb, cx+xx, cy+yy)) ++kern_black;
        }
        // Klassifikation:
        //  - fast keine schwarzen Randpixel -> leerer Platz (>goal nicht erreicht)
        //  - viel Rand, Kern voll schwarz -> massiv gefuellt
        //  - viel Rand, Kern hell -> Umriss ODER gefuellt-mit-hellem-Kern
        if (rand_black <= 2) { ++leer; }
        else if (kern_black >= 7) { ++solid; }
        else {
            // Kern hell: Umriss (offen, unter Ziel) oder ueber-Limit-Markierung.
            // Unterscheidung ueber die Fuellung zwischen Rand und Kern: bei der
            // ueber-Limit-Scheibe ist der Bereich Rand..Kern schwarz (Kranz),
            // beim reinen Umriss nur eine duenne Linie am Rand.
            int mid_black = 0;
            const int qx = dx + dot/4;  // Viertelposition (zwischen Rand und Mitte)
            for (int yy = dots_y+2; yy < dots_y+dot-2; ++yy)
                if (PortraitBlack(fb, qx, yy)) ++mid_black;
            if (mid_black >= 4) ++ring; else ++outline;
        }
    }
    printf("Gemessen: %d massiv gefuellt, %d gefuellt-mit-Kern (ueber Limit), %d Umriss (offen), %d leer\n",
           solid, ring, outline, leer);

    // PGM schreiben (portrait, damit man es normal ansieht)
    FILE* f = fopen(pgm, "wb");
    fprintf(f, "P5\n%d %d\n255\n", kPortW, kPortH);
    for (int y = 0; y < kPortH; ++y)
        for (int x = 0; x < kPortW; ++x) {
            uint8_t v = PortraitBlack(fb, x, y) ? 0 : 255;
            fwrite(&v, 1, 1, f);
        }
    fclose(f);
    printf("PGM: %s (%dx%d)\n\n", pgm, kPortW, kPortH);
}

int main()
{
    // Drei bekannte Faelle als Kreuzprobe der Klassifikation:
    analyse(2, 4, "/tmp/card_2.pgm");  // unter Ziel
    analyse(4, 4, "/tmp/card_4.pgm");  // genau Ziel
    analyse(6, 4, "/tmp/card.pgm");    // ueber Ziel
    return 0;
}
