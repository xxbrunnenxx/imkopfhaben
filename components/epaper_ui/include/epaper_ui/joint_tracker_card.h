#ifndef EPAPER_UI_JOINT_TRACKER_CARD_H_
#define EPAPER_UI_JOINT_TRACKER_CARD_H_

#include <cstdint>
#include <string>

#include "design_tokens.h"
#include "epaper_ui/overlay_geometry.h"

namespace epaper_ui {

// Der 420-Track auf der Startseite: ein Label, der Zaehlstand als Text und eine
// Reihe Punkte. Gefuellte Punkte sind bereits gezaehlt, offene Punkte der Rest
// bis zum Tagesziel. Ueber dem Ziel kommen zusaetzliche gefuellte Punkte mit
// einem Ring dazu (markiert, nicht limitiert). Fokussiert wird die Karte
// hervorgehoben, damit sichtbar ist, dass Up/Down jetzt zaehlen.
struct JointTrackerCardState {
    std::string label_text = "420-Track";
    int count = 0;   // heute gezaehlt (>= 0)
    int goal = 4;    // Tagesziel
    bool focused = false;   // Regler steht drauf -> Zeile invertiert (wie Menue)
    bool counting = false;  // Zaehlmodus aktiv -> zurueck-invertiert, Punkte fuellbar

    bool operator==(const JointTrackerCardState& other) const = default;
};

struct JointTrackerCardStyle {
    design::TypographyRole label_role = design::TypographyRole::kLabelMediumBlack;
    design::TypographyRole count_role = design::TypographyRole::kLabelSmallBlack;
    uint8_t text_color = design::color::kBlack;
    uint8_t dot_color = design::color::kBlack;
    uint8_t background_color = design::color::kWhite;
    // Invertiert (Regler steht drauf): wie eine ausgewaehlte Menuezeile.
    uint8_t selected_background_color = design::color::kBlack;
    uint8_t selected_text_color = design::color::kWhite;
    uint8_t border_color = design::color::kBlack;
    int width = 0;
    int dot_diameter = 14;
    int dot_gap = design::spacing::k8;
    int label_gap = design::spacing::k4;   // Abstand Label -> Punktreihe
    int top_border_thickness = design::menu_item::kBottomBorderThickness;
    int horizontal_padding = design::menu_item::kHorizontalPadding;
    int padding = design::spacing::k8;
};

UiRect JointTrackerCardBounds(int origin_x,
                              int origin_y,
                              const JointTrackerCardState& state,
                              const JointTrackerCardStyle& style);
void DrawJointTrackerCard(uint8_t* framebuffer,
                          int raw_width,
                          int raw_height,
                          int portrait_width,
                          int portrait_height,
                          int origin_x,
                          int origin_y,
                          const JointTrackerCardState& state,
                          const JointTrackerCardStyle& style);

}  // namespace epaper_ui

#endif  // EPAPER_UI_JOINT_TRACKER_CARD_H_
