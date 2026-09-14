#ifndef FOOTER_RUNTIME_H_
#define FOOTER_RUNTIME_H_

#include <cstdint>
#include <mutex>

#include "app_interaction_result.h"
#include "app_interaction_target.h"
#include "display_service.h"
#include "epaper_ui/global_footer.h"
#include "esp_err.h"
#include "page_navigation/navigation_model.h"

namespace footer_runtime {

enum class FooterFocusItem : uint8_t {
    kNone = 0,
    kHome,
    kSettings,
    kWifi,
    kFolder,
    kMic,
    kSticky,
};

struct LayoutState {
    bool visible = true;
    bool show_home = false;
    bool show_settings = false;
    bool show_wifi = false;
    bool show_folder = false;
    bool show_mic = true;
    bool show_sticky = false;
};

// Future page ports should project shared page focus into this state instead of
// letting the footer own a standalone navigation index.
struct ProjectionState {
    FooterFocusItem focused_item = FooterFocusItem::kNone;
};

using ActivateHandler = app_interaction::InputResult (*)(FooterFocusItem item, void* context);

// Maps a page's own footer item_index (as wired by AddFooterNavigationItems, see
// navigation_model.cpp) to the corresponding footer icon -- shared by every page runtime's
// focus-projection logic so the mapping is defined in exactly one place.
FooterFocusItem FooterItemForSelectedIndex(int selected_index);
page_navigation::NavigationItemRole FooterRoleForFooterItem(FooterFocusItem item);

void SetActivateHandler(ActivateHandler handler, void* context);
void SetLayoutState(const LayoutState& state);
void SetProjectionState(const ProjectionState& state);
LayoutState GetLayoutState();
ProjectionState GetProjectionState();

epaper_ui::GlobalFooterState BuildState();
esp_err_t UpdateDisplayState();
esp_err_t UpdateDisplayStateAndRequestRefresh(
    display_service::RefreshMode refresh_mode = display_service::RefreshMode::kPartial);
esp_err_t UpdateDisplayStateAndRefreshNow(
    display_service::RefreshMode refresh_mode = display_service::RefreshMode::kPartial);

}  // namespace footer_runtime

#endif  // FOOTER_RUNTIME_H_
