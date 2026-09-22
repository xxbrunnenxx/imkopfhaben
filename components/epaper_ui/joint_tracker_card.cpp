#include "epaper_ui/joint_tracker_card.h"

#include <algorithm>
#include <string>

#include "epaper_ui/font_renderer.h"
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

// Hoehe der Labelzeile - wie eine Menuezeile aus Text + oben/unten Padding.
int LabelRowHeight(const JointTrackerCardStyle& style)
{
    return LineHeight(style.label_role) + (2 * ClampPositive(style.row_vertical_padding));
}

int ContentHeight(const JointTrackerCardStyle& style)
{
    const int dot = ClampPositive(style.dot_diameter);
    return LabelRowHeight(style) + ClampPositive(style.label_gap) + dot;
}

}  // namespace

UiRect JointTrackerCardBounds(int origin_x,
                              int origin_y,
                              const JointTrackerCardState&,
                              const JointTrackerCardStyle& style)
{
    const int width = ClampPositive(style.width);
    const int height = ContentHeight(style);
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

    // Grundflaeche immer weiss uebermalen, damit ein alter Stand verschwindet.
    FillPortraitRect(framebuffer, raw_width, raw_height, portrait_width, portrait_height, bounds,
                     style.background_color);

    const int radius = ClampPositive(style.corner_radius);
    const int hpad = ClampPositive(style.horizontal_padding);

    // Die Labelzeile ist invertiert, solange der Regler daraufsteht und wir NICHT
    // im Zaehlmodus sind - genau wie eine ausgewaehlte Menuezeile. Sobald gezaehlt
    // wird, springt sie zurueck auf weiss (Punkte werden "aktiv" gefuellt).
    const bool row_inverted = state.focused && !state.counting;
    const uint8_t row_bg =
        row_inverted ? style.selected_background_color : style.background_color;
    const uint8_t row_text =
        row_inverted ? style.selected_text_color : style.text_color;

    // Obere Trennlinie - dieselbe Optik wie der bottom_border der Menuezeilen.
    const int border = ClampPositive(style.top_border_thickness);
    if (border > 0) {
        const int inset = std::min(radius, bounds.width / 2);
        const int separator_width = std::max(0, bounds.width - (2 * inset));
        if (separator_width > 0) {
            const UiRect separator = {bounds.x + inset, bounds.y, separator_width,
                                      std::min(border, bounds.height)};
            FillPortraitRect(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                             separator, style.border_color);
        }
    }

    // Labelzeile (Text links, Zaehlstand rechts), bei Fokus invertiert.
    const int label_row_height = LabelRowHeight(style);
    const UiRect label_row = {bounds.x, bounds.y, bounds.width, label_row_height};
    if (row_inverted) {
        FillRoundedPortraitRect(framebuffer, raw_width, raw_height, portrait_width,
                                portrait_height, label_row, radius, row_bg);
    }

    const int content_width = std::max(0, bounds.width - (2 * hpad));
    const int label_baseline_y =
        label_row.y + CenterOffset(label_row_height, LineHeight(style.label_role));

    DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                       bounds.x + hpad, label_baseline_y, state.label_text, style.label_role,
                       row_text);

    const std::string count_text =
        std::to_string(std::max(0, state.count)) + " / " + std::to_string(std::max(0, state.goal));
    const int count_width = MeasureText(style.count_role, count_text);
    const int count_x = bounds.x + hpad + std::max(0, content_width - count_width);
    const int count_baseline_y =
        label_row.y + CenterOffset(label_row_height, LineHeight(style.count_role));
    DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                       count_x, count_baseline_y, count_text, style.count_role, row_text);

    // Punktreihe in der Zeile DRUNTER - immer auf weissem Grund (sie liegt
    // ausserhalb der invertierten Labelzeile).
    const int dot = ClampPositive(style.dot_diameter);
    const int dot_gap = ClampPositive(style.dot_gap);
    const int dots_y = label_row.bottom() + ClampPositive(style.label_gap);
    const int total = TotalDots(state);
    const int filled = std::max(0, state.count);

    int dot_x = bounds.x + hpad;
    const int dots_right = bounds.x + hpad + content_width;
    for (int i = 0; i < total; ++i) {
        // Ausserhalb der Kartenbreite nicht mehr zeichnen (Schutz bei sehr vielen
        // Punkten ueber Limit).
        if (dot_x + dot > dots_right) {
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
