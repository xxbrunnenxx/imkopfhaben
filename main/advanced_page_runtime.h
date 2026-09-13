#ifndef ADVANCED_PAGE_RUNTIME_H_
#define ADVANCED_PAGE_RUNTIME_H_

#include "advanced_page_interactions.h"
#include "app_interaction_target.h"
#include "display_service.h"
#include "esp_err.h"
#include "footer_runtime.h"
#include "page_action_result.h"

namespace advanced_page_runtime {

esp_err_t UpdateDisplayState();
esp_err_t UpdateDisplayStateAndRequestRefresh(
    display_service::RefreshMode refresh_mode = display_service::RefreshMode::kPartial);
esp_err_t UpdateDisplayStateAndRequestRefresh(
    const display_service::RefreshRequest& refresh_request);
page_actions::FocusMoveOutcome MoveFocus(int delta);
advanced_page_interactions::ActivateResult ActivateFocusedItem();

footer_runtime::ProjectionState BuildFooterProjectionState();
page_actions::FocusUpdateOutcome FocusFooterItem(footer_runtime::FooterFocusItem item);
void ResetFocus();

// Deferred navigation from the Settings "Advanced" button, mirroring the onboarding
// manual-launch pattern: page_input_runtime can't call app_shell's ShowAdvancedScreen
// directly (app_shell depends on it, not the other way around), so it sets a pending
// flag here instead, which app_shell polls right after input dispatch.
void RequestLaunch();
bool ConsumePendingLaunch();

}  // namespace advanced_page_runtime

#endif  // ADVANCED_PAGE_RUNTIME_H_
