#include "advanced_page_coordinator.h"

#include <cstdio>
#include <string>

#include "audio_settings_service.h"

namespace {

// Anzeigetext fuer die aktuelle Lautstaerke: 0 % liest sich als "Mute".
std::string FormatVolumeValue(int volume)
{
    if (volume <= 0) {
        return "Mute";
    }
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "%d %%", volume);
    return std::string(buffer);
}

std::string FormatStorageBytes(uint64_t bytes)
{
    constexpr uint64_t kKilobyte = 1024ULL;
    constexpr uint64_t kMegabyte = 1024ULL * kKilobyte;
    constexpr uint64_t kGigabyte = 1024ULL * kMegabyte;

    char buffer[32] = {};
    if (bytes >= kGigabyte) {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%.1f GB free",
                      static_cast<double>(bytes) / static_cast<double>(kGigabyte));
    } else if (bytes >= kMegabyte) {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%.1f MB free",
                      static_cast<double>(bytes) / static_cast<double>(kMegabyte));
    } else if (bytes >= kKilobyte) {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%.1f KB free",
                      static_cast<double>(bytes) / static_cast<double>(kKilobyte));
    } else {
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%llu B free",
                      static_cast<unsigned long long>(bytes));
    }
    return std::string(buffer);
}

}  // namespace

AdvancedPageCoordinator::AdvancedPageCoordinator() = default;

void AdvancedPageCoordinator::Show()
{
    focus_.Configure(navigation_model_.item_count, 0);
    volume_editing_ = false;
}

bool AdvancedPageCoordinator::MoveFocus(int delta)
{
    return focus_.Move(delta);
}

bool AdvancedPageCoordinator::SetFocusIndex(int index)
{
    return focus_.SetIndex(index);
}

bool AdvancedPageCoordinator::IsRoleFocused(page_navigation::NavigationItemRole role) const
{
    return navigation_model_.IsRoleSelected(focus_.index(), role);
}

epaper_ui::AdvancedPageState AdvancedPageCoordinator::BuildState(
    const storage_service::Snapshot& storage_snapshot) const
{
    storage_service::StorageStats storage_stats = {};
    const bool allow_live_storage_stats =
        !storage_service::IsWriteBusy() &&
        storage_snapshot.mode != storage_service::Mode::kFormatting;
    const bool has_storage_stats =
        allow_live_storage_stats && storage_service::GetStorageStats(&storage_stats);

    epaper_ui::AdvancedPageState state = {};
    state.navigation_focus_index = focus_.index();
    state.title_text = "Advanced";

    state.storage_status.has_sd_card =
        storage_snapshot.inserted && storage_snapshot.mounted && has_storage_stats;
    if (state.storage_status.has_sd_card) {
        state.storage_status.free_space_text = FormatStorageBytes(storage_stats.free_bytes);
        state.storage_status.used_percent = storage_stats.used_percent;
    }

    // Label tracks the mode so the button reads correctly if the page is revisited while
    // OTG is active or mid-transition.
    std::string_view otg_label = "Enable OTG";
    if (storage_snapshot.mode == storage_service::Mode::kUsbMounted) {
        otg_label = "Disable OTG";
    } else if (storage_snapshot.mode == storage_service::Mode::kEnteringUsbMode) {
        otg_label = "Enabling OTG";
    } else if (storage_snapshot.mode == storage_service::Mode::kExitingUsbMode) {
        otg_label = "Disabling OTG";
    }
    state.enable_otg_button = {
        .label_text = otg_label,
        .selected =
            IsRoleFocused(page_navigation::NavigationItemRole::kAdvancedEnableOtgButton),
    };

    std::string_view format_label = "Format SD";
    if (storage_snapshot.mode == storage_service::Mode::kFormatting ||
        (storage_snapshot.operation == storage_service::Operation::kFormatSd &&
         storage_snapshot.phase == storage_service::OperationPhase::kStarted)) {
        format_label = "Formatting SD";
    }
    state.format_sd_button = {
        .label_text = format_label,
        .selected =
            IsRoleFocused(page_navigation::NavigationItemRole::kAdvancedFormatSdButton),
    };
    state.manual_onboarding_button = {
        .label_text = "Manual",
        .selected = IsRoleFocused(
            page_navigation::NavigationItemRole::kAdvancedManualOnboardingButton),
    };

    const bool volume_focused =
        IsRoleFocused(page_navigation::NavigationItemRole::kAdvancedVolumeSelect);
    epaper_ui::SelectInputState volume_select = {};
    volume_select.label_text = volume_editing_ ? "Volume  (Up/Down)" : "Volume";
    volume_select.value_text = FormatVolumeValue(audio_settings_service::GetVolume());
    // Im Einstell-Modus bleibt das Feld hervorgehoben, damit sichtbar ist, dass
    // Up/Down jetzt die Lautstaerke stellen und nicht den Fokus bewegen.
    volume_select.focused = volume_focused || volume_editing_;
    state.volume_select = volume_select;
    return state;
}
