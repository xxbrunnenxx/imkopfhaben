#include "dashboard_page_runtime.h"

#include <climits>
#include <mutex>

#include "dashboard_page_coordinator.h"
#include "epaper_ui/dashboard_page.h"
#include "esp_log.h"
#include "joint_tracker_service.h"
#include "overlay_runtime.h"
#include "page_navigation/page_focus_projection.h"
#include "recording_archive_service.h"
#include "shared_page_interactions.h"
#include "ui_refresh_runtime.h"

namespace dashboard_page_runtime {
namespace {

constexpr const char* kTag = "DashboardPageRuntime";

std::mutex s_mutex;
DashboardPageCoordinator s_coordinator = {};
int32_t s_interaction_generation = 1;
uint32_t s_last_welcome_period = 0;
bool s_welcome_period_valid = false;
MenuItemHandler s_menu_item_handler = nullptr;
void* s_menu_item_context = nullptr;

void AdvanceInteractionGenerationLocked()
{
    if (s_interaction_generation == INT32_MAX) {
        s_interaction_generation = 1;
    } else {
        ++s_interaction_generation;
    }
}

epaper_ui::DashboardPageState BuildStateLocked()
{
    return s_coordinator.BuildState();
}

footer_runtime::ProjectionState BuildFooterProjectionStateLocked()
{
    return shared_page_interactions::BuildFooterProjectionStateForSection(
        s_coordinator, page_navigation::NavigationItemSection::kDashboardPageMenu);
}

bool FooterProjectionChangedForFocusIndexes(int old_focus_index, int new_focus_index)
{
    return shared_page_interactions::FooterProjectionChangedForSection(
        s_coordinator, page_navigation::NavigationItemSection::kDashboardPageMenu, old_focus_index,
        new_focus_index);
}

}  // namespace

esp_err_t UpdateDisplayState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return display_service::SetDashboardPageState(BuildStateLocked());
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
    return ui_refresh_runtime::Schedule(ui_refresh_runtime::SurfaceKey::kDashboardPage,
                                        &UpdateDisplayState, refresh_request);
}

page_actions::FocusMoveOutcome MoveFocus(int delta)
{
    page_actions::FocusMoveOutcome result = {};
    int old_focus_index = -1;
    int new_focus_index = -1;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        old_focus_index = s_coordinator.focus().index();
        result = dashboard_page_interactions::HandleMoveFocus(s_coordinator, delta);
        if (!result.handled) {
            return result;
        }
        new_focus_index = s_coordinator.focus().index();
    }
    result.sync_footer_projection =
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);
    return result;
}

dashboard_page_interactions::ActivateResult ActivateFocusedItem()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return dashboard_page_interactions::HandlePrimaryActivate(s_coordinator);
}

bool IsJointTrackerCounting()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_coordinator.joint_tracker_counting();
}

void ToggleJointTrackerCounting()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.ToggleJointTrackerCounting();
    }
    (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
}

JointCountMoveResult AdjustJointCountForMove(int delta)
{
    JointCountMoveResult result = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_coordinator.joint_tracker_counting()) {
            return result;
        }
        result.handled = true;
    }

    // Navigation: Up = -1, Down = +1. Hochzaehlen soll bei Up passieren.
    const int before = joint_tracker_service::GetTodayCount();
    const int after = (delta < 0) ? joint_tracker_service::Increment()
                                  : joint_tracker_service::Decrement();
    result.changed = (after != before);

    (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
    return result;
}

footer_runtime::ProjectionState BuildFooterProjectionState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return BuildFooterProjectionStateLocked();
}

page_actions::FocusUpdateOutcome FocusFooterItem(footer_runtime::FooterFocusItem item)
{
    page_actions::FocusUpdateOutcome result = {};
    const page_navigation::NavigationItemRole role = footer_runtime::FooterRoleForFooterItem(item);
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
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);
    return result;
}

void ResetFocus()
{
    footer_runtime::ProjectionState projection = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.PrepareForShow();
        AdvanceInteractionGenerationLocked();
        projection = BuildFooterProjectionStateLocked();
    }
    footer_runtime::SetProjectionState(projection);
}

esp_err_t SyncFromService(bool request_refresh_if_active)
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.RefreshFromArchive(recording_archive_service::GetSnapshot());
        // Re-baseline the rotation so RefreshWelcomeIfRotated only fires on later rollovers.
        s_last_welcome_period = DashboardPageCoordinator::WelcomePeriodsSinceEpoch();
        s_welcome_period_valid = true;
    }
    const esp_err_t err =
        request_refresh_if_active
            ? UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial)
            : UpdateDisplayState();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(kTag, "Dashboard sync failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t RefreshWelcomeIfRotated()
{
    const uint32_t period = DashboardPageCoordinator::WelcomePeriodsSinceEpoch();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_welcome_period_valid && period == s_last_welcome_period) {
            return ESP_OK;  // still in the same interval; nothing to repaint
        }
        s_last_welcome_period = period;
        s_welcome_period_valid = true;
    }
    return UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
}

void SetMenuItemHandler(MenuItemHandler handler, void* context)
{
    s_menu_item_handler = handler;
    s_menu_item_context = context;
}

void OpenMenuItem(int menu_index)
{
    if (s_menu_item_handler != nullptr && s_menu_item_handler(menu_index, s_menu_item_context)) {
        return;
    }
    ESP_LOGI(kTag, "Menu item %d (%s) not yet implemented", menu_index,
             epaper_ui::DashboardMenuItemLabel(menu_index));
    epaper_ui::ToastState toast = {};
    toast.visible = true;
    toast.body_text = "Coming soon";
    (void)overlay_runtime::ShowToastForDuration(toast, 1500);
}

}  // namespace dashboard_page_runtime
