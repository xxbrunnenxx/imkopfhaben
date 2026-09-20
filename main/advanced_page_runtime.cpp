#include "advanced_page_runtime.h"

#include <climits>
#include <mutex>

#include "advanced_page_coordinator.h"
#include "advanced_page_interactions.h"
#include "audio_settings_service.h"
#include "page_navigation/navigation_model.h"
#include "page_navigation/page_focus_projection.h"
#include "shared_page_interactions.h"
#include "storage_service.h"
#include "ui_refresh_runtime.h"
#include "waveshare_board.h"

namespace advanced_page_runtime {
namespace {

std::mutex s_mutex;
AdvancedPageCoordinator s_coordinator = {};
int32_t s_interaction_generation = 1;
bool s_pending_launch = false;

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
        s_coordinator, page_navigation::NavigationItemSection::kAdvancedPageMenu);
}

bool FooterProjectionChangedForFocusIndexes(int old_focus_index, int new_focus_index)
{
    return shared_page_interactions::FooterProjectionChangedForSection(
        s_coordinator, page_navigation::NavigationItemSection::kAdvancedPageMenu, old_focus_index,
        new_focus_index);
}

epaper_ui::AdvancedPageState BuildStateLocked()
{
    return s_coordinator.BuildState(storage_service::GetSnapshot());
}

}  // namespace

esp_err_t UpdateDisplayState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return display_service::SetAdvancedPageState(BuildStateLocked());
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
        ui_refresh_runtime::SurfaceKey::kAdvancedPage, &UpdateDisplayState, refresh_request);
}

page_actions::FocusMoveOutcome MoveFocus(int delta)
{
    page_actions::FocusMoveOutcome result = {};
    int old_focus_index = -1;
    int new_focus_index = -1;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        old_focus_index = s_coordinator.focus().index();
        result = advanced_page_interactions::HandleMoveFocus(s_coordinator, delta);
        if (!result.handled) {
            return result;
        }
        new_focus_index = s_coordinator.focus().index();
    }

    result.sync_footer_projection =
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);
    return result;
}

advanced_page_interactions::ActivateResult ActivateFocusedItem()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return advanced_page_interactions::HandlePrimaryActivate(s_coordinator);
}

bool IsVolumeEditing()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_coordinator.volume_editing();
}

void ToggleVolumeEditing()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.ToggleVolumeEditing();
    }
    (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
}

VolumeMoveResult AdjustVolumeForMove(int delta)
{
    VolumeMoveResult result = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_coordinator.volume_editing()) {
            return result;
        }
        result.handled = true;

        // Navigation: Up liefert delta -1, Down +1. Lauter soll bei Up passieren,
        // also gegen die Navigationsrichtung auf den Stufen-Index abbilden.
        const int step = (delta < 0) ? 1 : -1;
        const int new_index = audio_settings_service::GetVolumeIndex() + step;
        result.changed = audio_settings_service::SetVolumeIndex(new_index);
        if (result.changed) {
            audio_settings_service::ApplyVolumeToCodec(waveshare_board::GetAudioCodec());
        }
    }

    if (result.handled) {
        (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
    }
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
        s_coordinator.Show();
        AdvanceInteractionGenerationLocked();
        projection = BuildFooterProjectionStateLocked();
    }
    footer_runtime::SetProjectionState(projection);
}

void RequestLaunch()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_pending_launch = true;
}

bool ConsumePendingLaunch()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_pending_launch) {
        return false;
    }
    s_pending_launch = false;
    return true;
}

}  // namespace advanced_page_runtime
