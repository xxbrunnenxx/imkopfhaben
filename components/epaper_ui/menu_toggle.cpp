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

// Toggle labels are static string literals set once by the owning coordinator, so the fitted
// role/text for a given (address, width) pair never changes across redraws. Small fixed-size
// cache avoids repeating the same MeasureText/FitTextToWidth work on every redraw -- keyed by
// the label's data() pointer rather than its content, since that's cheaper and label text
// storage is stable for the lifetime of the process.
struct LabelFitCache {
    const char* label_data = nullptr;
    int max_width = -1;
    design::TypographyRole role = design::TypographyRole::kLabelLargeBlack;
    std::string fitted_text;
};

constexpr size_t kLabelFitCacheSlots = 4;
LabelFitCache s_label_fit_cache[kLabelFitCacheSlots];
size_t s_label_fit_cache_next = 0;

void FitLabelCached(std::string_view text,
                    design::TypographyRole preferred_role,
                    int max_width,
                    design::TypographyRole* out_role,
                    std::string* out_text)
{
    for (const LabelFitCache& slot : s_label_fit_cache) {
        if (slot.label_data == text.data() && slot.max_width == max_width) {
            *out_role = slot.role;
            *out_text = slot.fitted_text;
            return;
        }
    }

    *out_role = FitLabelRole(preferred_role, text, max_width);
    *out_text = FitTextToWidth(*out_role, text, max_width);

    LabelFitCache& slot = s_label_fit_cache[s_label_fit_cache_next];
    slot.label_data = text.data();
    slot.max_width = max_width;
    slot.role = *out_role;
    slot.fitted_text = *out_text;
    s_label_fit_cache_next = (s_label_fit_cache_next + 1) % kLabelFitCacheSlots;
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
    design::TypographyRole label_role = style.role;
    std::string label_text;
    FitLabelCached(state.label_text, style.role, label_max_width, &label_role, &label_text);

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
