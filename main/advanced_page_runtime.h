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

// True, solange der Lautstaerke-Einstell-Modus laeuft (Up/Down stellen dann die
// Lautstaerke statt den Fokus zu bewegen).
bool IsVolumeEditing();

// Schaltet den Lautstaerke-Einstell-Modus um und frischt die Anzeige auf.
void ToggleVolumeEditing();

struct VolumeMoveResult {
    bool handled = false;  // true = wir waren im Einstell-Modus, der Move ist verbraucht
    bool changed = false;  // true = die Lautstaerke hat sich tatsaechlich geaendert
};

// Verstellt die Lautstaerke, wenn der Einstell-Modus aktiv ist. delta folgt der
// Navigation (Up = -1, Down = +1); Up macht lauter. Wendet die neue Stufe sofort
// auf den Codec an und frischt die Anzeige auf.
VolumeMoveResult AdjustVolumeForMove(int delta);

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
