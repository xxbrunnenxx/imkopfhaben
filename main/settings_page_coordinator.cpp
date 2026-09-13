#include "settings_page_coordinator.h"

SettingsPageCoordinator::SettingsPageCoordinator() = default;

void SettingsPageCoordinator::Show()
{
    focus_.Configure(navigation_model_.item_count, 0);
}

bool SettingsPageCoordinator::MoveFocus(int delta)
{
    return focus_.Move(delta);
}

bool SettingsPageCoordinator::SetFocusIndex(int index)
{
    return focus_.SetIndex(index);
}

bool SettingsPageCoordinator::IsRoleFocused(page_navigation::NavigationItemRole role) const
{
    return navigation_model_.IsRoleSelected(focus_.index(), role);
}

void SettingsPageCoordinator::RefreshTimezoneFromService(
    const timezone_service::Snapshot& snapshot,
    const std::vector<timezone_service::TimezoneInfo>& timezones)
{
    timezones_ = timezones;
    timezone_name_ = snapshot.settings.timezone_name;
    timezone_description_ = timezone_name_;
    for (const timezone_service::TimezoneInfo& info : timezones_) {
        if (info.name == timezone_name_) {
            timezone_description_ = info.description.empty() ? info.name : info.description;
            break;
        }
    }
}

int SettingsPageCoordinator::SelectedTimezoneIndex() const
{
    for (size_t index = 0; index < timezones_.size(); ++index) {
        if (timezones_[index].name == timezone_name_) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void SettingsPageCoordinator::SetTimezoneByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(timezones_.size())) {
        return;
    }
    timezone_name_ = timezones_[static_cast<size_t>(index)].name;
    timezone_description_ = timezones_[static_cast<size_t>(index)].description.empty()
                                ? timezones_[static_cast<size_t>(index)].name
                                : timezones_[static_cast<size_t>(index)].description;

    timezone_service::SettingsPatch patch = {};
    patch.has_enabled = true;
    patch.enabled = true;
    patch.has_timezone_name = true;
    patch.timezone_name = timezone_name_;
    (void)timezone_service::ApplySettingsPatch(patch);
}

epaper_ui::SettingsPageState SettingsPageCoordinator::BuildState(
    const wifi_service::UiState& wifi_state) const
{
    epaper_ui::SettingsPageState state = {};
    state.navigation_focus_index = focus_.index();
    state.title_text = "Settings";
    state.wifi_toggle = {
        .label_text = "WiFi",
        .toggle_state = BuildToggleState(
            wifi_state.wifi_enabled,
            IsRoleFocused(page_navigation::NavigationItemRole::kSettingsWifiToggle)),
    };
    state.access_point_toggle = {
        .label_text = "Access Point",
        .toggle_state = BuildToggleState(
            wifi_state.access_point_mode,
            IsRoleFocused(page_navigation::NavigationItemRole::kSettingsEnableApToggle)),
    };
    state.playback_toggle = {
        .label_text = "Play back after recording",
        .toggle_state = BuildToggleState(
            recording_session_service::GetPlaybackAfterRecordingEnabled(),
            IsRoleFocused(page_navigation::NavigationItemRole::kSettingsPlaybackToggle)),
    };

    state.timezone = {
        .label_text = "Timezone",
        .placeholder_text = "Select timezone",
        .value_text = timezone_description_,
        .focused = IsRoleFocused(page_navigation::NavigationItemRole::kSettingsTimezoneField),
    };
    state.sync_now_button = {
        .label_text = "Sync now",
        .selected = IsRoleFocused(page_navigation::NavigationItemRole::kSettingsSyncNowButton),
    };
    state.advanced_button = {
        .label_text = "Advanced",
        .selected = IsRoleFocused(page_navigation::NavigationItemRole::kSettingsAdvancedButton),
    };
    return state;
}

epaper_ui::ToggleVisualState SettingsPageCoordinator::BuildToggleState(bool enabled, bool focused)
{
    if (focused) {
        return enabled ? epaper_ui::ToggleVisualState::kFocusOn
                       : epaper_ui::ToggleVisualState::kFocusOff;
    }
    return enabled ? epaper_ui::ToggleVisualState::kOn
                   : epaper_ui::ToggleVisualState::kOff;
}
