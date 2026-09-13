#include "epaper_ui/settings_page.h"

#include <algorithm>

#include "render_utils.h"

namespace epaper_ui {
namespace {

constexpr auto kTitleRole = design::TypographyRole::kHeadingH1;
constexpr auto kSectionRole = design::TypographyRole::kHeadingH2;
constexpr int kSideInset = design::spacing::k16;
constexpr int kTopGap = design::spacing::k16;
constexpr int kTitleBottomGap = design::spacing::k24;
constexpr int kNetworkHeadingGap = 0;
constexpr int kSectionGap = design::spacing::k24;
constexpr int kButtonStackGap = design::spacing::k12;
constexpr int kColumnGap = design::spacing::k12;

struct Layout {
    UiRect wifi_toggle = {};
    UiRect access_point_toggle = {};
    UiRect playback_toggle = {};
    UiRect timezone = {};
    UiRect sync_now_button = {};
    UiRect advanced_button = {};
};

Layout BuildLayout(int portrait_width, int portrait_height, const SettingsPageState& state)
{
    (void)portrait_height;
    const int page_x = kSideInset;
    const int page_width = std::max(0, portrait_width - (2 * kSideInset));
    const int title_y = StatusBarHeight() + kTopGap;
    const int title_height = LineHeight(kTitleRole);

    const int network_heading_y = title_y + title_height + kTitleBottomGap;
    const int network_items_y =
        network_heading_y + LineHeight(kSectionRole) + kNetworkHeadingGap;

    MenuToggleStyle network_toggle_style = {};
    network_toggle_style.width = page_width;
    const UiRect wifi_toggle = MenuToggleBounds(page_x, network_items_y, network_toggle_style);

    MenuToggleStyle access_point_style = network_toggle_style;
    access_point_style.bottom_border_thickness = 0;
    const UiRect access_point_toggle =
        MenuToggleBounds(page_x, wifi_toggle.bottom(), access_point_style);

    const int recording_heading_y = access_point_toggle.bottom() + kSectionGap;
    const int recording_items_y =
        recording_heading_y + LineHeight(kSectionRole) + kNetworkHeadingGap;
    MenuToggleStyle playback_toggle_style = network_toggle_style;
    const UiRect playback_toggle =
        MenuToggleBounds(page_x, recording_items_y, playback_toggle_style);

    // No separate section heading here: the select field already renders its own
    // "Timezone" label above the value, so a heading would just duplicate it and
    // waste a line of vertical space. Timezone and its sync button share one row
    // (like the old time page's hour/minute split) instead of stacking, since
    // this page doesn't scroll and every row counts.
    const int time_field_y = playback_toggle.bottom() + kSectionGap;
    const int sync_button_width = std::max(0, (page_width - kColumnGap) / 3);
    const int timezone_width = std::max(0, page_width - sync_button_width - kColumnGap);
    TextInputStyle timezone_style = {};
    timezone_style.width = timezone_width;
    const UiRect timezone = SelectInputBounds(page_x, time_field_y, state.timezone, timezone_style);

    // Align the button with the field itself, not the label text above it.
    const int timezone_label_offset =
        state.timezone.label_text.empty()
            ? 0
            : LineHeight(timezone_style.label_role) + timezone_style.label_gap;
    ButtonStyle sync_now_style = {};
    sync_now_style.width = sync_button_width;
    sync_now_style.height = timezone_style.field_height;
    const UiRect sync_now_button = ButtonBounds(page_x + timezone_width + kColumnGap,
                                                time_field_y + timezone_label_offset,
                                                state.sync_now_button, sync_now_style);

    // Storage status + maintenance actions (OTG/Format SD/Manual onboarding) moved to their
    // own Advanced page -- this page's fixed layout has no scrolling, and there wasn't room
    // left for all of it once Timezone/Sync moved in from the old time page.
    ButtonStyle advanced_button_style = {};
    advanced_button_style.width = page_width;
    const int advanced_button_y =
        std::max(timezone.bottom(), sync_now_button.bottom()) + kSectionGap;
    const UiRect advanced_button =
        ButtonBounds(page_x, advanced_button_y, state.advanced_button, advanced_button_style);

    return {
        .wifi_toggle = wifi_toggle,
        .access_point_toggle = access_point_toggle,
        .playback_toggle = playback_toggle,
        .timezone = timezone,
        .sync_now_button = sync_now_button,
        .advanced_button = advanced_button,
    };
}

}  // namespace

UiRect SettingsPageItemBounds(int portrait_width,
                              int portrait_height,
                              const SettingsPageState& state,
                              SettingsPageItemId item)
{
    const Layout layout = BuildLayout(portrait_width, portrait_height, state);
    switch (item) {
        case SettingsPageItemId::kWifiToggle:
            return layout.wifi_toggle;
        case SettingsPageItemId::kAccessPointToggle:
            return layout.access_point_toggle;
        case SettingsPageItemId::kPlaybackToggle:
            return layout.playback_toggle;
        case SettingsPageItemId::kTimezoneField:
            return layout.timezone;
        case SettingsPageItemId::kSyncNowButton:
            return layout.sync_now_button;
        case SettingsPageItemId::kAdvancedButton:
            return layout.advanced_button;
        case SettingsPageItemId::kNone:
        default:
            return {};
    }
}

UiRect SettingsPageItemVisualBounds(int portrait_width,
                                    int portrait_height,
                                    const SettingsPageState& state,
                                    SettingsPageItemId item)
{
    return SettingsPageItemBounds(portrait_width, portrait_height, state, item);
}

bool HitTestSettingsPageItem(int portrait_width,
                             int portrait_height,
                             const SettingsPageState& state,
                             int x,
                             int y,
                             SettingsPageItemId* item)
{
    if (item != nullptr) {
        *item = SettingsPageItemId::kNone;
    }

    constexpr SettingsPageItemId kItems[] = {
        SettingsPageItemId::kWifiToggle,
        SettingsPageItemId::kAccessPointToggle,
        SettingsPageItemId::kPlaybackToggle,
        SettingsPageItemId::kTimezoneField,
        SettingsPageItemId::kSyncNowButton,
        SettingsPageItemId::kAdvancedButton,
    };
    for (SettingsPageItemId candidate : kItems) {
        const UiRect bounds =
            SettingsPageItemBounds(portrait_width, portrait_height, state, candidate);
        if (!bounds.IsEmpty() && bounds.Contains(x, y)) {
            if (item != nullptr) {
                *item = candidate;
            }
            return true;
        }
    }

    return false;
}

void DrawSettingsPage(uint8_t* framebuffer,
                      int raw_width,
                      int raw_height,
                      int portrait_width,
                      int portrait_height,
                      const SettingsPageState& state,
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
                       layout.wifi_toggle.y - kNetworkHeadingGap - LineHeight(kSectionRole),
                       "Network",
                       kSectionRole,
                       design::color::kBlack);

    MenuToggleStyle wifi_style = {};
    wifi_style.width = layout.wifi_toggle.width;
    wifi_style.height = layout.wifi_toggle.height;
    DrawMenuToggle(framebuffer,
                   raw_width,
                   raw_height,
                   portrait_width,
                   portrait_height,
                   layout.wifi_toggle.x,
                   layout.wifi_toggle.y,
                   state.wifi_toggle,
                   wifi_style);

    MenuToggleStyle access_point_style = wifi_style;
    access_point_style.bottom_border_thickness = 0;
    DrawMenuToggle(framebuffer,
                   raw_width,
                   raw_height,
                   portrait_width,
                   portrait_height,
                   layout.access_point_toggle.x,
                   layout.access_point_toggle.y,
                   state.access_point_toggle,
                   access_point_style);

    DrawTypographyText(framebuffer,
                       raw_width,
                       raw_height,
                       portrait_width,
                       portrait_height,
                       title_x,
                       layout.playback_toggle.y - kNetworkHeadingGap - LineHeight(kSectionRole),
                       "Recording",
                       kSectionRole,
                       design::color::kBlack);

    MenuToggleStyle playback_style = {};
    playback_style.width = layout.playback_toggle.width;
    playback_style.height = layout.playback_toggle.height;
    DrawMenuToggle(framebuffer,
                   raw_width,
                   raw_height,
                   portrait_width,
                   portrait_height,
                   layout.playback_toggle.x,
                   layout.playback_toggle.y,
                   state.playback_toggle,
                   playback_style);

    TextInputStyle timezone_style = {};
    timezone_style.width = layout.timezone.width;
    DrawSelectInput(framebuffer,
                    raw_width,
                    raw_height,
                    portrait_width,
                    portrait_height,
                    layout.timezone.x,
                    layout.timezone.y,
                    state.timezone,
                    timezone_style);

    ButtonStyle sync_now_style = {};
    sync_now_style.width = layout.sync_now_button.width;
    sync_now_style.height = layout.sync_now_button.height;
    DrawButton(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               layout.sync_now_button.x,
               layout.sync_now_button.y,
               state.sync_now_button,
               sync_now_style);

    ButtonStyle advanced_button_style = {};
    advanced_button_style.width = layout.advanced_button.width;
    DrawButton(framebuffer,
               raw_width,
               raw_height,
               portrait_width,
               portrait_height,
               layout.advanced_button.x,
               layout.advanced_button.y,
               state.advanced_button,
               advanced_button_style);

    DrawGlobalFooter(framebuffer,
                     raw_width,
                     raw_height,
                     portrait_width,
                     portrait_height,
                     footer_state);
}

}  // namespace epaper_ui
