#include "epaper_ui/advanced_page.h"

#include <algorithm>

#include "render_utils.h"

namespace epaper_ui {
namespace {

constexpr auto kTitleRole = design::TypographyRole::kHeadingH1;
constexpr auto kSectionRole = design::TypographyRole::kHeadingH2;
constexpr int kSideInset = design::spacing::k16;
constexpr int kTopGap = design::spacing::k16;
constexpr int kTitleBottomGap = design::spacing::k24;
constexpr int kStorageStatusGap = design::spacing::k12;
constexpr int kStorageButtonTopGap = design::spacing::k16;
constexpr int kButtonStackGap = design::spacing::k12;

struct Layout {
    UiRect storage_status = {};
    UiRect enable_otg_button = {};
    UiRect format_sd_button = {};
    UiRect manual_onboarding_button = {};
};

Layout BuildLayout(int portrait_width, int portrait_height, const AdvancedPageState& state)
{
    (void)portrait_height;
    const int page_x = kSideInset;
    const int page_width = std::max(0, portrait_width - (2 * kSideInset));
    const int title_y = StatusBarHeight() + kTopGap;
    const int title_height = LineHeight(kTitleRole);

    const int storage_heading_y = title_y + title_height + kTitleBottomGap;
    SdStatusStyle storage_style = {};
    storage_style.max_width = page_width;
    const UiRect storage_status = SdStatusBounds(page_x,
                                                 storage_heading_y + LineHeight(kSectionRole) +
                                                     kStorageStatusGap,
                                                 state.storage_status,
                                                 storage_style);

    ButtonStyle otg_button_style = {};
    otg_button_style.width = page_width;
    const int button_y = storage_status.bottom() + kStorageButtonTopGap;
    const UiRect enable_otg_button =
        ButtonBounds(page_x, button_y, state.enable_otg_button, otg_button_style);

    ButtonStyle format_button_style = {};
    format_button_style.width = page_width;
    const UiRect format_sd_button =
        ButtonBounds(page_x, enable_otg_button.bottom() + kButtonStackGap,
                     state.format_sd_button, format_button_style);

    ButtonStyle manual_button_style = {};
    manual_button_style.width = page_width;
    const UiRect manual_onboarding_button =
        ButtonBounds(page_x, format_sd_button.bottom() + kButtonStackGap,
                     state.manual_onboarding_button, manual_button_style);

    return {
        .storage_status = storage_status,
        .enable_otg_button = enable_otg_button,
        .format_sd_button = format_sd_button,
        .manual_onboarding_button = manual_onboarding_button,
    };
}

}  // namespace

UiRect AdvancedPageItemBounds(int portrait_width,
                              int portrait_height,
                              const AdvancedPageState& state,
                              AdvancedPageItemId item)
{
    const Layout layout = BuildLayout(portrait_width, portrait_height, state);
    switch (item) {
        case AdvancedPageItemId::kEnableOtgButton:
            return layout.enable_otg_button;
        case AdvancedPageItemId::kFormatSdButton:
            return layout.format_sd_button;
        case AdvancedPageItemId::kManualOnboardingButton:
            return layout.manual_onboarding_button;
        case AdvancedPageItemId::kNone:
        default:
            return {};
    }
}

UiRect AdvancedPageItemVisualBounds(int portrait_width,
                                    int portrait_height,
                                    const AdvancedPageState& state,
                                    AdvancedPageItemId item)
{
    return AdvancedPageItemBounds(portrait_width, portrait_height, state, item);
}

bool HitTestAdvancedPageItem(int portrait_width,
                             int portrait_height,
                             const AdvancedPageState& state,
                             int x,
                             int y,
                             AdvancedPageItemId* item)
{
    if (item != nullptr) {
        *item = AdvancedPageItemId::kNone;
    }

    constexpr AdvancedPageItemId kItems[] = {
        AdvancedPageItemId::kEnableOtgButton,
        AdvancedPageItemId::kFormatSdButton,
        AdvancedPageItemId::kManualOnboardingButton,
    };
    for (AdvancedPageItemId candidate : kItems) {
        const UiRect bounds =
            AdvancedPageItemBounds(portrait_width, portrait_height, state, candidate);
        if (!bounds.IsEmpty() && bounds.Contains(x, y)) {
            if (item != nullptr) {
                *item = candidate;
            }
            return true;
        }
    }

    return false;
}

void DrawAdvancedPage(uint8_t* framebuffer,
                      int raw_width,
                      int raw_height,
                      int portrait_width,
                      int portrait_height,
                      const AdvancedPageState& state,
                      const StatusBarState& status_bar_state,
                      const GlobalFooterState& footer_state)
{
    if (framebuffer == nullptr) {
        return;
    }

    FillPortraitRect(framebuffer,
                     raw_width,
                     raw_height,
                     portrait_width,
                     portrait_height,
                     {0, 0, portrait_width, portrait_height},
                     design::color::kWhite);
    DrawStatusBar(framebuffer,
                  raw_width,
                  raw_height,
                  portrait_width,
                  portrait_height,
                  status_bar_state);

    const Layout layout = BuildLayout(portrait_width, portrait_height, state);
    const int title_x = kSideInset;
    const int title_y = StatusBarHeight() + kTopGap;
    DrawTypographyText(framebuffer,
                       raw_width,
                       raw_height,
                       portrait_width,
                       portrait_height,
                       title_x,
                       title_y,
                       state.title_text,
                       kTitleRole,
                       design::color::kBlack);

    DrawTypographyText(framebuffer,
                       raw_width,
                       raw_height,
                       portrait_width,
                       portrait_height,
                       title_x,
                       layout.storage_status.y - kStorageStatusGap - LineHeight(kSectionRole),
                       "Storage",
                       kSectionRole,
                       design::color::kBlack);

    SdStatusStyle storage_style = {};
    storage_style.max_width = layout.storage_status.width;
    DrawSdStatus(framebuffer,
                 raw_width,
                 raw_height,
                 portrait_width,
                 portrait_height,
                 layout.storage_status.x,
                 layout.storage_status.y,
                 state.storage_status,
                 storage_style);

    ButtonStyle otg_button_style = {};
    otg_button_style.width = layout.enable_otg_button.width;
    DrawButton(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               layout.enable_otg_button.x,
               layout.enable_otg_button.y,
               state.enable_otg_button,
               otg_button_style);

    ButtonStyle format_button_style = {};
    format_button_style.width = layout.format_sd_button.width;
    format_button_style.variant = ButtonVariant::kPrimary;
    DrawButton(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               layout.format_sd_button.x,
               layout.format_sd_button.y,
               state.format_sd_button,
               format_button_style);

    ButtonStyle manual_button_style = {};
    manual_button_style.width = layout.manual_onboarding_button.width;
    DrawButton(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               layout.manual_onboarding_button.x,
               layout.manual_onboarding_button.y,
               state.manual_onboarding_button,
               manual_button_style);

    DrawGlobalFooter(framebuffer,
                     raw_width,
                     raw_height,
                     portrait_width,
                     portrait_height,
                     footer_state);
}

}  // namespace epaper_ui
