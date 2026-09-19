#include "epaper_ui/welcome_message.h"

#include <algorithm>
#include <array>
#include <vector>

#include "project_assets.h"
#include "render_utils.h"

namespace epaper_ui {
namespace {

constexpr std::array<const char*, 5> kTitles = {
    "What's on your mind?", "Let's jump back in!", "Got a cool idea?",
    "Capture that thought!", "What's the plan?",
};

const EmbeddedImageAsset* TitleIcon(const WelcomeMessageStyle& style)
{
    return style.show_icon ? project_assets::GetIcon(EmbeddedIconId::kStar) : nullptr;
}

int WrapWidthForIcon(const WelcomeMessageStyle& style, const EmbeddedImageAsset* icon)
{
    int max_width = style.title_max_width;
    if (max_width > 0 && icon != nullptr) {
        max_width = std::max(1, max_width - ClampPositive(style.icon_gap) - icon->width);
    }
    return max_width;
}

}  // namespace

std::string WelcomeMessageTitle(uint32_t seed)
{
    return kTitles[seed % kTitles.size()];
}

int WelcomeMessageTitleCount()
{
    return static_cast<int>(kTitles.size());
}

namespace {

// Zeilen pro Spalte bei zweispaltigem Satz: die erste Haelfte links, der Rest rechts.
int InfoRowsPerColumn(const WelcomeMessageState& state)
{
    const int count = static_cast<int>(state.info_lines.size());
    return (count + 1) / 2;  // aufrunden: bei ungerader Zahl steht links eine mehr
}

// Height of the status block (info_lines): laid out in two columns, so the height is
// rows-per-column lines separated by info_line_gap. Zero when there are no lines.
int InfoBlockHeight(const WelcomeMessageState& state, const WelcomeMessageStyle& style)
{
    if (state.info_lines.empty()) {
        return 0;
    }
    const int line_height = std::max(1, LineHeight(style.info_role));
    const int rows = InfoRowsPerColumn(state);
    return rows * line_height + (rows - 1) * ClampPositive(style.info_line_gap);
}

// Full width of the two-column block: left column width + gap + right column width.
int InfoBlockWidth(const WelcomeMessageState& state, const WelcomeMessageStyle& style)
{
    const int count = static_cast<int>(state.info_lines.size());
    if (count == 0) {
        return 0;
    }
    const int rows = InfoRowsPerColumn(state);
    int left = 0;
    int right = 0;
    for (int i = 0; i < count; ++i) {
        const int w = MeasureText(style.info_role, state.info_lines[i]);
        if (i < rows) {
            left = std::max(left, w);
        } else {
            right = std::max(right, w);
        }
    }
    if (right == 0) {
        return left;
    }
    return left + ClampPositive(style.info_column_gap) + right;
}

}  // namespace

UiRect WelcomeMessageBounds(int origin_x,
                            int origin_y,
                            const WelcomeMessageState& state,
                            const WelcomeMessageStyle& style)
{
    const UiRect date_bounds =
        CurrentDateBounds(origin_x, origin_y, state.current_date, style.current_date);
    const bool has_date = !state.current_date.weekday_text.empty() ||
                          !state.current_date.date_text.empty();

    // Status block replaces the greeting when info_lines is set.
    if (!state.info_lines.empty()) {
        const int info_height = InfoBlockHeight(state, style);
        const int info_width = InfoBlockWidth(state, style);
        const int gap = has_date ? ClampPositive(style.section_gap) : 0;
        return {origin_x, origin_y, std::max(date_bounds.width, info_width),
                date_bounds.height + gap + info_height};
    }

    const EmbeddedImageAsset* icon = TitleIcon(style);
    const std::vector<std::string> lines =
        WrapTextToWidth(style.title_role, state.title_text, WrapWidthForIcon(style, icon));
    const int line_height = std::max(1, LineHeight(style.title_role));
    const int title_height = static_cast<int>(lines.size()) * line_height;

    int title_width = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        int line_width = MeasureText(style.title_role, lines[i]);
        if (i + 1 == lines.size() && icon != nullptr) {
            line_width += ClampPositive(style.icon_gap) + icon->width;
        }
        title_width = std::max(title_width, line_width);
    }

    const int gap = (has_date && !lines.empty()) ? ClampPositive(style.section_gap) : 0;
    return {origin_x, origin_y, std::max(date_bounds.width, title_width),
            date_bounds.height + gap + title_height};
}

void DrawWelcomeMessage(uint8_t* framebuffer,
                        int raw_width,
                        int raw_height,
                        int portrait_width,
                        int portrait_height,
                        int origin_x,
                        int origin_y,
                        const WelcomeMessageState& state,
                        const WelcomeMessageStyle& style)
{
    DrawCurrentDate(framebuffer, raw_width, raw_height, portrait_width, portrait_height, origin_x,
                    origin_y, state.current_date, style.current_date);

    const UiRect date_bounds =
        CurrentDateBounds(origin_x, origin_y, state.current_date, style.current_date);
    const bool has_date = !state.current_date.weekday_text.empty() ||
                          !state.current_date.date_text.empty();
    const int title_y = origin_y + date_bounds.height + (has_date ? ClampPositive(style.section_gap)
                                                                  : 0);

    // Status block: two columns. First half of the entries goes into the left column, the rest
    // into the right, each top to bottom. Halves the height so the menu below stays put.
    if (!state.info_lines.empty()) {
        const int info_line_height = std::max(1, LineHeight(style.info_role));
        const int row_step = info_line_height + ClampPositive(style.info_line_gap);
        const int count = static_cast<int>(state.info_lines.size());
        const int rows = InfoRowsPerColumn(state);

        // Left column width drives where the right column starts.
        int left_width = 0;
        for (int i = 0; i < rows && i < count; ++i) {
            left_width = std::max(left_width, MeasureText(style.info_role, state.info_lines[i]));
        }
        const int right_x = origin_x + left_width + ClampPositive(style.info_column_gap);

        for (int i = 0; i < count; ++i) {
            const bool left = i < rows;
            const int col_x = left ? origin_x : right_x;
            const int row = left ? i : (i - rows);
            const int line_y = title_y + row * row_step;
            DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                               col_x, line_y, state.info_lines[i], style.info_role,
                               style.info_color);
        }
        return;
    }

    const EmbeddedImageAsset* icon = TitleIcon(style);
    const std::vector<std::string> lines =
        WrapTextToWidth(style.title_role, state.title_text, WrapWidthForIcon(style, icon));
    const int line_height = std::max(1, LineHeight(style.title_role));

    int cursor_y = title_y;
    int last_line_width = 0;
    for (const std::string& line : lines) {
        DrawTypographyText(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                           origin_x, cursor_y, line, style.title_role, style.title_color);
        last_line_width = MeasureText(style.title_role, line);
        cursor_y += line_height;
    }

    if (icon == nullptr || lines.empty()) {
        return;
    }

    const int last_line_y = title_y + (static_cast<int>(lines.size()) - 1) * line_height;
    DrawPortraitMonoAsset(framebuffer, raw_width, raw_height, portrait_width, portrait_height,
                          origin_x + last_line_width + ClampPositive(style.icon_gap),
                          last_line_y + CenterOffset(line_height, icon->height) +
                              style.icon_y_offset,
                          icon, style.icon_color);
}

}  // namespace epaper_ui
