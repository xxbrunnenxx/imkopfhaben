#ifndef ADVANCED_PAGE_INTERACTIONS_H_
#define ADVANCED_PAGE_INTERACTIONS_H_

#include <cstdint>
#include <functional>

#include "advanced_page_coordinator.h"
#include "page_action_result.h"

namespace advanced_page_interactions {

enum class ActivateIntent : uint8_t {
    kNone = 0,
    kShowHome,
    kShowSettings,
    kShowWifi,
    kEnableOtg,
    kShowFormatSdModal,
    kShowOnboarding,
};

struct ActivateResult {
    ActivateIntent intent = ActivateIntent::kNone;
    bool handled = false;
    bool play_activate_cue = false;
};

using FocusMoveResult = page_actions::FocusMoveOutcome;

struct ActivateCallbacks {
    std::function<void()> show_home;
    std::function<void()> show_settings;
    std::function<void()> show_wifi;
    std::function<void()> enable_otg;
    std::function<void()> show_format_sd_modal;
    std::function<void()> show_onboarding;
};

ActivateResult HandlePrimaryActivate(const AdvancedPageCoordinator& coordinator);
void ApplyPrimaryActivateResult(const ActivateResult& result,
                               const ActivateCallbacks& callbacks);
FocusMoveResult HandleMoveFocus(AdvancedPageCoordinator& coordinator, int delta);

}  // namespace advanced_page_interactions

#endif  // ADVANCED_PAGE_INTERACTIONS_H_
