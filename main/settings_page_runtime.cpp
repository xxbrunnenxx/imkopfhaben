#include "settings_page_runtime.h"

#include <climits>
#include <mutex>

#include "esp_log.h"
#include "overlay_runtime.h"
#include "page_navigation/navigation_model.h"
#include "page_navigation/page_focus_projection.h"
#include "project_assets.h"
#include "settings_page_interactions.h"
#include "settings_page_coordinator.h"
#include "shared_page_interactions.h"
#include "timezone_service.h"
#include "ui_refresh_runtime.h"
#include "wifi_service.h"

namespace settings_page_runtime {
namespace {

constexpr const char* kTag = "SettingsPageRuntime";

std::mutex s_mutex;
SettingsPageCoordinator s_coordinator = {};
int32_t s_interaction_generation = 1;
bool s_timezone_modal_active = false;

void AdvanceInteractionGenerationLocked()
{
    if (s_interaction_generation == INT32_MAX) {
        s_interaction_generation = 1;
    } else {
        ++s_interaction_generation;
    }
}

footer_runtime::ProjectionState BuildFooterProjectionStateLocked()
{
    return shared_page_interactions::BuildFooterProjectionStateForSection(
        s_coordinator, page_navigation::NavigationItemSection::kSettingsPageMenu);
}

bool FooterProjectionChangedForFocusIndexes(int old_focus_index, int new_focus_index)
{
    return shared_page_interactions::FooterProjectionChangedForSection(
        s_coordinator, page_navigation::NavigationItemSection::kSettingsPageMenu, old_focus_index,
        new_focus_index);
}

epaper_ui::SettingsPageState BuildStateLocked()
{
    s_coordinator.RefreshTimezoneFromService(timezone_service::GetSnapshot(),
                                             timezone_service::ListTimezones());
    return s_coordinator.BuildState(wifi_service::GetUiState());
}

}  // namespace

esp_err_t UpdateDisplayState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return display_service::SetSettingsPageState(BuildStateLocked());
}

esp_err_t UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode refresh_mode)
{
    return UpdateDisplayStateAndRequestRefresh(display_service::RefreshRequest{
        .refresh_mode = refresh_mode,
    });
}

esp_err_t UpdateDisplayStateAndRequestRefresh(
    const display_service::RefreshRequest& refresh_request)
{
    return ui_refresh_runtime::Schedule(
        ui_refresh_runtime::SurfaceKey::kSettingsPage, &UpdateDisplayState, refresh_request);
}

page_actions::FocusMoveOutcome MoveFocus(int delta)
{
    page_actions::FocusMoveOutcome result = {};
    int old_focus_index = -1;
    int new_focus_index = -1;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        old_focus_index = s_coordinator.focus().index();
        result = settings_page_interactions::HandleMoveFocus(s_coordinator, delta);
        if (!result.handled) {
            return result;
        }
        new_focus_index = s_coordinator.focus().index();
    }

    result.sync_footer_projection =
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);    return result;
}

settings_page_interactions::ActivateResult ActivateFocusedItem()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return settings_page_interactions::HandlePrimaryActivate(s_coordinator);
}

footer_runtime::ProjectionState BuildFooterProjectionState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return BuildFooterProjectionStateLocked();
}

page_actions::FocusUpdateOutcome FocusFooterItem(footer_runtime::FooterFocusItem item)
{
    page_actions::FocusUpdateOutcome result = {};
    const page_navigation::NavigationItemRole role =
        footer_runtime::FooterRoleForFooterItem(item);
    if (role == page_navigation::NavigationItemRole::kUnknown) {
        return result;
    }

    int old_focus_index = -1;
    int new_focus_index = -1;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        const int focus_index = s_coordinator.navigation_model().IndexOfRole(role);
        if (focus_index < 0) {
            return result;
        }
        old_focus_index = s_coordinator.focus().index();
        if (!s_coordinator.SetFocusIndex(focus_index)) {
            return result;
        }
        new_focus_index = s_coordinator.focus().index();
    }

    result.handled = true;
    result.apply_page_state = true;
    result.sync_footer_projection =
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);    return result;
}

void ResetFocus()
{
    footer_runtime::ProjectionState projection = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.Show();
        AdvanceInteractionGenerationLocked();
        projection = BuildFooterProjectionStateLocked();
    }
    footer_runtime::SetProjectionState(projection);
}

esp_err_t ShowTimezoneModal()
{
    epaper_ui::SelectModalState state = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        state.visible = true;
        state.title_text = "Select timezone";
        const int selected = s_coordinator.SelectedTimezoneIndex();
        state.selected_index = selected < 0 ? 0 : selected;
        for (const timezone_service::TimezoneInfo& tz : s_coordinator.timezones()) {
            state.items.push_back({
                .label_text = timezone_service::DisplayDescription(tz),
            });
        }
        s_timezone_modal_active = true;
    }
    return overlay_runtime::ShowSelectModal(state);
}

bool HandleSelectModalSubmit(int selected_index)
{
    bool was_active = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        was_active = s_timezone_modal_active;
        s_timezone_modal_active = false;
        if (was_active) {
            s_coordinator.SetTimezoneByIndex(selected_index);
        }
    }
    if (was_active) {
        (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
    }
    return was_active;
}

void ClearPendingSelectModal()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_timezone_modal_active = false;
}

esp_err_t SyncTimeNow()
{
    const bool success = timezone_service::SyncNow();
    epaper_ui::ToastState toast = {};
    toast.visible = true;
    toast.body_text = success ? "Synced" : "Sync failed";
    toast.leading_icon = project_assets::GetIcon(EmbeddedIconId::kTime);
    (void)overlay_runtime::ShowToastForDuration(toast, 2000);
    return success ? ESP_OK : ESP_FAIL;
}

}  // namespace settings_page_runtime
