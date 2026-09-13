#include "epaper_ui/menu_toggle.h"

#include <algorithm>
#include <array>
#include <string>

#include "render_utils.h"

namespace epaper_ui {
namespace {

// Shrinks the label to whichever role in this same-weight ladder fits max_width, falling
// back to hard truncation at the smallest size if even that overflows. Short labels ("WiFi")
// are unaffected -- they already fit at the first (largest) role tried.
design::TypographyRole FitLabelRole(design::TypographyRole preferred_role,
                                    std::string_view text,
                                    int max_width)
{
    if (MeasureText(preferred_role, text) <= max_width) {
        return preferred_role;
    }

    constexpr std::array<design::TypographyRole, 3> kShrinkLadder = {
        design::TypographyRole::kLabelLargeBlack,
        design::TypographyRole::kLabelMediumBlack,
        design::TypographyRole::kLabelSmallBlack,
    };
    for (design::TypographyRole role : kShrinkLadder) {
        if (MeasureText(role, text) <= max_width) {
            return role;
        }
    }
    return kShrinkLadder.back();
}

std::string FitLabelText(std::string_view text, design::TypographyRole role, int max_width)
{
    if (text.empty() || max_width <= 0 || MeasureText(role, text) <= max_width) {
        return std::string(text);
    }

    size_t length = text.size();
    while (length > 0) {
        std::string_view candidate = text.substr(0, length);
        if (MeasureText(role, candidate) <= max_width) {
            return std::string(candidate);
        }
        --length;
    }
    return {};
}

}  // namespace

UiRect MenuToggleBounds(int origin_x, int origin_y, const MenuToggleStyle& style)
{
    return {origin_x, origin_y, ClampPositive(style.width), ClampPositive(style.height)};
}

void DrawMenuToggle(uint8_t* framebuffer,
                    int raw_width,
                    int raw_height,
                    int portrait_width,
                    int portrait_height,
                    int origin_x,
                    int origin_y,
                    const MenuToggleState& state,
                    const MenuToggleStyle& style)
{
    const UiRect bounds = MenuToggleBounds(origin_x, origin_y, style);
    if (bounds.IsEmpty()) {
        return;
    }

    FillPortraitRect(framebuffer,
                     raw_width,
                     raw_height,
                     portrait_width,
                     portrait_height,
                     bounds,
                     style.background_color);

    const int border_height = ClampPositive(style.bottom_border_thickness);
    if (border_height > 0) {
        FillPortraitRect(framebuffer,
                         raw_width,
                         raw_height,
                         portrait_width,
                         portrait_height,
                         {bounds.x,
                          bounds.bottom() - std::min(border_height, bounds.height),
                          bounds.width,
                          std::min(border_height, bounds.height)},
                         style.border_color);
    }

    ToggleStyle toggle_style = style.toggle;
    const UiRect toggle_bounds = ToggleBounds(0, 0, toggle_style);
    const int toggle_x =
        bounds.right() - ClampPositive(style.horizontal_padding) - toggle_bounds.width;
    const int toggle_y = bounds.y + CenterOffset(bounds.height, toggle_bounds.height);
    DrawToggle(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               toggle_x,
               toggle_y,
               state.toggle_state,
               toggle_style);

    if (state.label_text.empty()) {
        return;
    }

    // The label must not run into the toggle: shrink it (same weight, smaller size) until it
    // fits the space left of the control, truncating only if even the smallest size overflows.
    const int label_max_width = std::max(
        0, toggle_x - ClampPositive(style.control_gap) - bounds.x -
               ClampPositive(style.horizontal_padding));
    const design::TypographyRole label_role =
        FitLabelRole(style.role, state.label_text, label_max_width);
    const std::string label_text = FitLabelText(state.label_text, label_role, label_max_width);

    DrawTypographyText(framebuffer,
                       raw_width,
                       raw_height,
                       portrait_width,
                       portrait_height,
                       bounds.x + ClampPositive(style.horizontal_padding),
                       bounds.y + CenterOffset(bounds.height, LineHeight(label_role)),
                       label_text,
                       label_role,
                       style.text_color);
}

}  // namespace epaper_ui
