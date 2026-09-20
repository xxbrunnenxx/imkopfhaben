#include "epaper_ui/joint_tracker_card.h"

#include <algorithm>
#include <string>

#include "render_utils.h"

namespace epaper_ui {
namespace {

// Wie viele Punkte insgesamt gezeichnet werden: mindestens das Tagesziel, bei
// Ueberschreitung so viele wie gezaehlt (die zusaetzlichen sind die "ueber
// Limit"-Punkte).
int TotalDots(const JointTrackerCardState& state)
{
    return std::max(std::max(state.goal, 0), std::max(state.count, 0));
}

int ContentHeight(const JointTrackerCardStyle& style)
{
    const int label_height = LineHeight(style.label_role);
    const int dot = ClampPositive(style.dot_diameter);
    return label_height + ClampPositive(style.label_gap) + dot;
}

}  // namespace

UiRect JointTrackerCardBounds(int origin_x,
                              int origin_y,
                              const JointTrackerCardState&,
                              const JointTrackerCardStyle& style)
{
    const int width = ClampPositive(style.width);
    const int padding = ClampPositive(style.padding);
    const int height = ContentHeight(style) + (2 * padding);
    return {origin_x, origin_y, width, height};
}

void DrawJointTrackerCard(uint8_t* framebuffer,
                          int raw_width,
                          int raw_height,
                          int portrait_width,
                          int portrait_height,
                          int origin_x,
                          int origin_y,
                          const JointTrackerCardState& state,
                          const JointTrackerCardStyle& style)
{
    const UiRect bounds = JointTrackerCardBounds(origin_x, origin_y, state, style);
    if (bounds.IsEmpty()) {
        return;
    }

    const int padding = ClampPositive(style.padding);
    const int radius = ClampPositive(style.corner_radius);

    // Hintergrund immer fuellen, damit ein alter Stand sauber uebermalt wird.
    FillRoundedPortraitRect(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                            bounds, radius, style.background_color);
    // Fokussiert bekommt die Karte einen Rahmen, sonst keiner.
    if (state.focused) {
        DrawRoundedPortraitBorder(framebuffer, raw_width, raw_height, portrait_width,
                                  portrait_height, bounds, radius,
                                  ClampPositive(style.border_thickness), style.dot_color);
    }

    const int content_x = origin_x + padding;
    const int content_y = origin_y + padding;
    const int content_width = std::max(0, bounds.width - (2 * padding));

    // Kopfzeile: Label links, Zaehlstand "n / goal" rechts.
    DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                       content_x, content_y, state.label_text, style.label_role, style.text_color);

    const std::string count_text =
        std::to_string(std::max(0, state.count)) + " / " + std::to_string(std::max(0, state.goal));
    const int count_width = MeasureText(style.count_role, count_text);
    const int count_x = content_x + std::max(0, content_width - count_width);
    DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                       count_x, content_y, count_text, style.count_role, style.text_color);

    // Punktreihe unter der Kopfzeile.
    const int dot = ClampPositive(style.dot_diameter);
    const int dot_gap = ClampPositive(style.dot_gap);
    const int dots_y = content_y + LineHeight(style.label_role) + ClampPositive(style.label_gap);
    const int total = TotalDots(state);
    const int filled = std::max(0, state.count);

    int dot_x = content_x;
    for (int i = 0; i < total; ++i) {
        // Ausserhalb der Kartenbreite nicht mehr zeichnen (Schutz bei sehr vielen
        // Punkten ueber Limit).
        if (dot_x + dot > content_x + content_width) {
            break;
        }
        const UiRect dot_rect = {dot_x, dots_y, dot, dot};
        const int dot_radius = dot / 2;
        if (i < filled) {
            // Gezaehlter Punkt: gefuellt.
            FillRoundedPortraitRect(framebuffer, raw_width, raw_height, portrait_width,
                                    portrait_height, dot_rect, dot_radius, style.dot_color);
            if (i >= state.goal) {
                // Ueber dem Tagesziel: zusaetzlich ein heller Kern als Markierung
                // ("markiert, nicht limitiert").
                const int inset = std::max(1, dot / 4);
                const UiRect core = {dot_x + inset, dots_y + inset, dot - (2 * inset),
                                     dot - (2 * inset)};
                FillRoundedPortraitRect(framebuffer, raw_width, raw_height, portrait_width,
                                        portrait_height, core, std::max(1, dot_radius - inset),
                                        style.background_color);
            }
        } else {
            // Noch offener Punkt bis zum Ziel: nur Umriss.
            DrawRoundedPortraitBorder(framebuffer, raw_width, raw_height, portrait_width,
                                      portrait_height, dot_rect, dot_radius, 1, style.dot_color);
        }
        dot_x += dot + dot_gap;
    }
}

}  // namespace epaper_ui
