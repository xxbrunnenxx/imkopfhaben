#ifndef EPAPER_UI_WELCOME_MESSAGE_H_
#define EPAPER_UI_WELCOME_MESSAGE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "epaper_ui/current_date.h"
#include "epaper_ui/overlay_geometry.h"

namespace epaper_ui {

// The dashboard header: the current date on top, then either a large greeting (legacy) or,
// when info_lines is set, a compact multi-line status block (device/brain info) with no icon.
struct WelcomeMessageState {
    CurrentDateState current_date = {};
    std::string title_text = {};
    // When non-empty, the status block replaces the greeting: each entry is one line, drawn in
    // info_role, top to bottom, no wrapping and no trailing icon.
    std::vector<std::string> info_lines = {};

    bool operator==(const WelcomeMessageState& other) const = default;
};

struct WelcomeMessageStyle {
    CurrentDateStyle current_date = {};
    design::TypographyRole title_role = design::TypographyRole::kDisplay;
    uint8_t title_color = design::color::kBlack;
    int title_max_width = 0;  // 0 = no wrapping (single line)
    int section_gap = design::welcome_message::kSectionGap;
    int icon_gap = design::welcome_message::kIconGap;
    int icon_y_offset = design::welcome_message::kIconYOffset;
    uint8_t icon_color = design::color::kBlack;
    bool show_icon = true;
    // Typography for the status block (info_lines). Smaller than the greeting so several lines
    // fit under the date.
    design::TypographyRole info_role = design::TypographyRole::kBody;
    uint8_t info_color = design::color::kBlack;
    int info_line_gap = design::spacing::k4;
};

// Returns the title variant text for a rotating greeting, indexed by seed.
std::string WelcomeMessageTitle(uint32_t seed);
int WelcomeMessageTitleCount();

UiRect WelcomeMessageBounds(int origin_x,
                            int origin_y,
                            const WelcomeMessageState& state,
                            const WelcomeMessageStyle& style);
void DrawWelcomeMessage(uint8_t* framebuffer,
                        int raw_width,
                        int raw_height,
                        int portrait_width,
                        int portrait_height,
                        int origin_x,
                        int origin_y,
                        const WelcomeMessageState& state,
                        const WelcomeMessageStyle& style);

}  // namespace epaper_ui

#endif  // EPAPER_UI_WELCOME_MESSAGE_H_
