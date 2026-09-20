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
    bool focused = false;

    bool operator==(const JointTrackerCardState& other) const = default;
};

struct JointTrackerCardStyle {
    design::TypographyRole label_role = design::TypographyRole::kLabelSmallBlack;
    design::TypographyRole count_role = design::TypographyRole::kLabelSmallBlack;
    uint8_t text_color = design::color::kBlack;
    uint8_t dot_color = design::color::kBlack;
    uint8_t background_color = design::status_bar::kBackgroundColor;
    int width = 0;
    int dot_diameter = 14;
    int dot_gap = design::spacing::k8;
    int label_gap = design::spacing::k4;   // Abstand Label -> Punktreihe
    int border_thickness = design::progress_bar::kBorderThickness;
    int corner_radius = 6;
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
