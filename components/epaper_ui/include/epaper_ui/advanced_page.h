#ifndef EPAPER_UI_ADVANCED_PAGE_H_
#define EPAPER_UI_ADVANCED_PAGE_H_

#include <cstdint>
#include <string_view>

#include "epaper_ui/button.h"
#include "epaper_ui/global_footer.h"
#include "epaper_ui/sd_status.h"
#include "epaper_ui/select_input.h"
#include "epaper_ui/status_bar.h"

namespace epaper_ui {

enum class AdvancedPageItemId : uint8_t {
    kNone = 0,
    kEnableOtgButton,
    kFormatSdButton,
    kManualOnboardingButton,
    kVolumeSelect,
};

struct AdvancedPageState {
    int navigation_focus_index = -1;
    std::string_view title_text = "Advanced";
    SdStatusState storage_status = {};
    ButtonState enable_otg_button = {};
    ButtonState format_sd_button = {};
    ButtonState manual_onboarding_button = {};
    SelectInputState volume_select = {};
};

UiRect AdvancedPageItemBounds(int portrait_width,
                              int portrait_height,
                              const AdvancedPageState& state,
                              AdvancedPageItemId item);
UiRect AdvancedPageItemVisualBounds(int portrait_width,
                                    int portrait_height,
                                    const AdvancedPageState& state,
                                    AdvancedPageItemId item);
bool HitTestAdvancedPageItem(int portrait_width,
                             int portrait_height,
                             const AdvancedPageState& state,
                             int x,
                             int y,
                             AdvancedPageItemId* item);
void DrawAdvancedPage(uint8_t* framebuffer,
                      int raw_width,
                      int raw_height,
                      int portrait_width,
                      int portrait_height,
                      const AdvancedPageState& state,
                      const StatusBarState& status_bar_state,
                      const GlobalFooterState& footer_state);

}  // namespace epaper_ui

#endif  // EPAPER_UI_ADVANCED_PAGE_H_
