#ifndef SETTINGS_PAGE_RUNTIME_H_
#define SETTINGS_PAGE_RUNTIME_H_

#include <cstdint>

#include "app_interaction_target.h"
#include "display_service.h"
#include "esp_err.h"
#include "footer_runtime.h"
#include "page_action_result.h"
#include "settings_page_interactions.h"

namespace settings_page_runtime {

esp_err_t UpdateDisplayState();
esp_err_t UpdateDisplayStateAndRequestRefresh(
    display_service::RefreshMode refresh_mode = display_service::RefreshMode::kPartial);
esp_err_t UpdateDisplayStateAndRequestRefresh(
    const display_service::RefreshRequest& refresh_request);
page_actions::FocusMoveOutcome MoveFocus(int delta);
settings_page_interactions::ActivateResult ActivateFocusedItem();

footer_runtime::ProjectionState BuildFooterProjectionState();
page_actions::FocusUpdateOutcome FocusFooterItem(footer_runtime::FooterFocusItem item);
void ResetFocus();

// Field activations (invoked by the page-input activate callbacks).
esp_err_t ShowTimezoneModal();
esp_err_t SyncTimeNow();
// Applies a timezone chosen from the select modal launched by ShowTimezoneModal(), but only
// if that modal is the one currently pending (mirrors the per-page claim chain in
// app_shell.cpp's HandleDispatchedButtonEvent). Returns true if it claimed the submission.
bool HandleSelectModalSubmit(int selected_index);
void ClearPendingSelectModal();

}  // namespace settings_page_runtime

#endif  // SETTINGS_PAGE_RUNTIME_H_
