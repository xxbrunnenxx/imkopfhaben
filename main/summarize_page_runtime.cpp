#include "summarize_page_runtime.h"

#include <climits>
#include <mutex>

#include "epaper_ui/summarize_page.h"
#include "esp_log.h"
#include "local_ai_service.h"
#include "page_navigation/page_focus_projection.h"
#include "shared_page_interactions.h"
#include "summarize_page_coordinator.h"
#include "ui_refresh_runtime.h"

namespace summarize_page_runtime {
namespace {

constexpr const char* kTag = "SummarizePageRuntime";

std::mutex s_mutex;
SummarizePageCoordinator s_coordinator = {};
summary_service::Snapshot s_summary_snapshot = {};
int32_t s_interaction_generation = 1;

void AdvanceInteractionGenerationLocked()
{
    if (s_interaction_generation == INT32_MAX) {
        s_interaction_generation = 1;
    } else {
        ++s_interaction_generation;
    }
}

epaper_ui::SummarizePageState BuildStateLocked()
{
    const bool local_ai_ready = local_ai_service::GetSnapshot().runtime.ready;
    return s_coordinator.BuildState(local_ai_ready, s_summary_snapshot);
}

footer_runtime::ProjectionState BuildFooterProjectionStateLocked()
{
    return shared_page_interactions::BuildFooterProjectionStateForSection(
        s_coordinator, page_navigation::NavigationItemSection::kSummarizePageControls);
}

bool FooterProjectionChangedForFocusIndexes(int old_focus_index, int new_focus_index)
{
    return shared_page_interactions::FooterProjectionChangedForSection(
        s_coordinator, page_navigation::NavigationItemSection::kSummarizePageControls,
        old_focus_index, new_focus_index);
}

}  // namespace

esp_err_t UpdateDisplayState()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return display_service::SetSummarizePageState(BuildStateLocked());
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
    return ui_refresh_runtime::Schedule(ui_refresh_runtime::SurfaceKey::kSummarizePage,
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
        result = summarize_page_interactions::HandleMoveFocus(s_coordinator, delta);
        if (!result.handled) {
            return result;
        }
        new_focus_index = s_coordinator.focus().index();
    }
    result.sync_footer_projection =
        FooterProjectionChangedForFocusIndexes(old_focus_index, new_focus_index);
    return result;
}

summarize_page_interactions::ActivateResult ActivateFocusedItem()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const bool local_ai_ready = local_ai_service::GetSnapshot().runtime.ready;
    return summarize_page_interactions::HandlePrimaryActivate(s_coordinator, local_ai_ready);
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
        s_coordinator.ExitSegmentControl();
        s_coordinator.ExitScrollContainer();
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
    const summary_service::Snapshot snapshot = summary_service::GetSnapshot();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_summary_snapshot = snapshot;
    }
    const esp_err_t err =
        request_refresh_if_active
            ? UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial)
            : UpdateDisplayState();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(kTag, "Summarize sync failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t OnSummarySnapshot(const summary_service::Snapshot& snapshot, bool request_refresh)
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_summary_snapshot = snapshot;
    }
    return request_refresh ? UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial)
                           : UpdateDisplayState();
}

void ToggleSegment()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_coordinator.segment_control_active()) {
            s_coordinator.ExitSegmentControl();
        } else {
            s_coordinator.EnterSegmentControl();
        }
    }
    (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
}

void EnterScroll()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_coordinator.EnterScrollContainer();
    }
    (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
}

void RequestNotesSummary()
{
    (void)summary_service::RequestSummary(summary_service::SummaryKind::kNotes);
}

void RequestTodosSummary()
{
    (void)summary_service::RequestSummary(summary_service::SummaryKind::kTodos);
}

bool ExitActiveControl()
{
    bool exited = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        exited = s_coordinator.ExitScrollContainer() || s_coordinator.ExitSegmentControl();
    }
    if (exited) {
        (void)UpdateDisplayStateAndRequestRefresh(display_service::RefreshMode::kPartial);
    }
    return exited;
}

}  // namespace summarize_page_runtime
