#ifndef SHARED_PAGE_INTERACTIONS_H_
#define SHARED_PAGE_INTERACTIONS_H_

#include "footer_runtime.h"
#include "page_action_result.h"
#include "page_navigation/navigation_model.h"
#include "page_navigation/page_focus_projection.h"

namespace shared_page_interactions {

template <typename ActivateResult, typename ActivateIntent, typename Coordinator>
ActivateResult HandleFooterPrimaryActivate(const Coordinator& coordinator,
                                          ActivateIntent show_home_intent,
                                          ActivateIntent show_settings_intent,
                                          ActivateIntent footer_wifi_intent)
{
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kFooterHome)) {
        return {
            .intent = show_home_intent,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kFooterSettings)) {
        return {
            .intent = show_settings_intent,
            .handled = true,
            .play_activate_cue = true,
        };
    }
    if (coordinator.IsRoleFocused(page_navigation::NavigationItemRole::kFooterWifi)) {
        return {
            .intent = footer_wifi_intent,
            .handled = true,
            .play_activate_cue = true,
        };
    }

    return {};
}

template <typename Coordinator>
page_actions::FocusMoveOutcome HandleMoveFocus(Coordinator& coordinator, int delta)
{
    if (delta == 0 || !coordinator.MoveFocus(delta)) {
        return {};
    }

    page_actions::FocusMoveOutcome outcome = {};
    outcome.handled = true;
    outcome.play_navigation_cue = true;
    outcome.apply_page_state = true;
    return outcome;
}

// Projects a page's own live focus index into the shared footer's focused item. Coordinator
// must expose navigation_model() and focus().index(); `section` is the page's own
// NavigationItemSection (its non-footer menu items), which ProjectPageFocus uses to tell "focus
// is on this page's own menu" apart from "focus is on the shared footer".
template <typename Coordinator>
footer_runtime::ProjectionState BuildFooterProjectionStateForSection(
    const Coordinator& coordinator, page_navigation::NavigationItemSection section)
{
    const page_navigation::PageFocusProjection projection = page_navigation::ProjectPageFocus(
        coordinator.navigation_model(), section, coordinator.focus().index(), -1, -1);
    footer_runtime::ProjectionState state = {};
    state.focused_item = footer_runtime::FooterItemForSelectedIndex(projection.footer_selected_index);
    return state;
}

// Same projection as above, evaluated at two focus indexes, to tell callers whether the footer's
// focused item actually changed (so they can skip re-syncing the footer when it didn't).
template <typename Coordinator>
bool FooterProjectionChangedForSection(const Coordinator& coordinator,
                                       page_navigation::NavigationItemSection section,
                                       int old_focus_index,
                                       int new_focus_index)
{
    const page_navigation::PageFocusProjection old_projection = page_navigation::ProjectPageFocus(
        coordinator.navigation_model(), section, old_focus_index, -1, -1);
    const page_navigation::PageFocusProjection new_projection = page_navigation::ProjectPageFocus(
        coordinator.navigation_model(), section, new_focus_index, -1, -1);
    return footer_runtime::FooterItemForSelectedIndex(old_projection.footer_selected_index) !=
          footer_runtime::FooterItemForSelectedIndex(new_projection.footer_selected_index);
}

}  // namespace shared_page_interactions

#endif  // SHARED_PAGE_INTERACTIONS_H_
