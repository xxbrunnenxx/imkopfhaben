#include "advanced_page_interactions.h"

#include "shared_page_interactions.h"

namespace advanced_page_interactions {

ActivateResult HandlePrimaryActivate(const AdvancedPageCoordinator& coordinator)
{
    const ActivateResult footer_result =
        shared_page_interactions::HandleFooterPrimaryActivate<ActivateResult>(
            coordinator,
            ActivateIntent::kShowHome,
            ActivateIntent::kShowSettings,
            ActivateIntent::kShowWifi);
    if (footer_result.handled) {
        return footer_result;
    }

    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kAdvancedEnableOtgButton)) {
        return {
            .intent = ActivateIntent::kEnableOtg,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kAdvancedFormatSdButton)) {
        return {
            .intent = ActivateIntent::kShowFormatSdModal,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(
            page_navigation::NavigationItemRole::kAdvancedManualOnboardingButton)) {
        return {
            .intent = ActivateIntent::kShowOnboarding,
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
        case ActivateIntent::kShowSettings:
            if (callbacks.show_settings) {
                callbacks.show_settings();
            }
            return;
        case ActivateIntent::kShowWifi:
            if (callbacks.show_wifi) {
                callbacks.show_wifi();
            }
            return;
        case ActivateIntent::kEnableOtg:
            if (callbacks.enable_otg) {
                callbacks.enable_otg();
            }
            return;
        case ActivateIntent::kShowFormatSdModal:
            if (callbacks.show_format_sd_modal) {
                callbacks.show_format_sd_modal();
            }
            return;
        case ActivateIntent::kShowOnboarding:
            if (callbacks.show_onboarding) {
                callbacks.show_onboarding();
            }
            return;
        case ActivateIntent::kNone:
        default:
            return;
    }
}

FocusMoveResult HandleMoveFocus(AdvancedPageCoordinator& coordinator, int delta)
{
    return shared_page_interactions::HandleMoveFocus(coordinator, delta);
}

}  // namespace advanced_page_interactions
