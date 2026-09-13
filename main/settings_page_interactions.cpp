#include "settings_page_interactions.h"

#include "shared_page_interactions.h"

namespace settings_page_interactions {

ActivateResult HandlePrimaryActivate(const SettingsPageCoordinator& coordinator)
{
    const ActivateResult footer_result =
        shared_page_interactions::HandleFooterPrimaryActivate<ActivateResult>(
            coordinator,
            ActivateIntent::kShowHome,
            ActivateIntent::kForceRefresh,
            ActivateIntent::kShowWifi);
    if (footer_result.handled) {
        return footer_result;
    }

    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kSettingsWifiToggle)) {
        return {
            .intent = ActivateIntent::kToggleWifi,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(
            page_navigation::NavigationItemRole::kSettingsEnableApToggle)) {
        return {
            .intent = ActivateIntent::kToggleAccessPoint,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(
            page_navigation::NavigationItemRole::kSettingsPlaybackToggle)) {
        return {
            .intent = ActivateIntent::kTogglePlayback,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kSettingsTimezoneField)) {
        return {
            .intent = ActivateIntent::kShowTimezoneModal,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kSettingsSyncNowButton)) {
        return {
            .intent = ActivateIntent::kSyncTimeNow,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kSettingsAdvancedButton)) {
        return {
            .intent = ActivateIntent::kShowAdvanced,
            .handled = true,
            .play_activate_cue = true,
        };
    }

    return {};
}

void ApplyPrimaryActivateResult(const ActivateResult& result,
                                const ActivateCallbacks& callbacks)
{
    if (!result.handled) {
        return;
    }

    switch (result.intent) {
        case ActivateIntent::kShowHome:
            if (callbacks.show_home) {
                callbacks.show_home();
            }
            return;
        case ActivateIntent::kShowWifi:
            if (callbacks.show_wifi) {
                callbacks.show_wifi();
            }
            return;
        case ActivateIntent::kShowTimezoneModal:
            if (callbacks.show_timezone_modal) {
                callbacks.show_timezone_modal();
            }
            return;
        case ActivateIntent::kSyncTimeNow:
            if (callbacks.sync_time_now) {
                callbacks.sync_time_now();
            }
            return;
        case ActivateIntent::kForceRefresh:
            if (callbacks.force_refresh) {
                callbacks.force_refresh();
            }
            return;
        case ActivateIntent::kToggleWifi:
            if (callbacks.toggle_wifi) {
                callbacks.toggle_wifi();
            }
            return;
        case ActivateIntent::kToggleAccessPoint:
            if (callbacks.toggle_access_point) {
                callbacks.toggle_access_point();
            }
            return;
        case ActivateIntent::kTogglePlayback:
            if (callbacks.toggle_playback) {
                callbacks.toggle_playback();
            }
            return;
        case ActivateIntent::kShowAdvanced:
            if (callbacks.show_advanced) {
                callbacks.show_advanced();
            }
            return;
        case ActivateIntent::kNone:
        default:
            return;
    }
}

FocusMoveResult HandleMoveFocus(SettingsPageCoordinator& coordinator, int delta)
{
    return shared_page_interactions::HandleMoveFocus(coordinator, delta);
}

}  // namespace settings_page_interactions
