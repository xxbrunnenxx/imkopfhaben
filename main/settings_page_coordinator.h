#ifndef SETTINGS_PAGE_COORDINATOR_H_
#define SETTINGS_PAGE_COORDINATOR_H_

#include <string>
#include <vector>

#include "epaper_ui/settings_page.h"
#include "page_navigation/navigation_model.h"
#include "page_navigation/roving_focus.h"
#include "recording_session_service.h"
#include "timezone_service.h"
#include "wifi_service.h"

class SettingsPageCoordinator {
public:
    SettingsPageCoordinator();

    void Show();
    bool MoveFocus(int delta);
    bool SetFocusIndex(int index);
    bool IsRoleFocused(page_navigation::NavigationItemRole role) const;

    // Keeps the timezone list and current-selection label in sync with the service. Called on
    // every page refresh. Unlike the former time page, selecting a timezone applies
    // immediately (see SetTimezoneByIndex) rather than staging an edit for a separate save
    // step, so there's no "user edited" guard to worry about here.
    void RefreshTimezoneFromService(const timezone_service::Snapshot& snapshot,
                                    const std::vector<timezone_service::TimezoneInfo>& timezones);

    epaper_ui::SettingsPageState BuildState(const wifi_service::UiState& wifi_state) const;

    const std::vector<timezone_service::TimezoneInfo>& timezones() const { return timezones_; }
    int SelectedTimezoneIndex() const;
    void SetTimezoneByIndex(int index);

    const page_navigation::NavigationModel& navigation_model() const { return navigation_model_; }
    const page_navigation::RovingFocus& focus() const { return focus_; }

private:
    static epaper_ui::ToggleVisualState BuildToggleState(bool enabled, bool focused);

    page_navigation::NavigationModel navigation_model_ =
        page_navigation::BuildSettingsPageNavigationModel();
    page_navigation::RovingFocus focus_{navigation_model_.item_count, 0};

    std::vector<timezone_service::TimezoneInfo> timezones_ = {};
    std::string timezone_name_ = {};
    std::string timezone_description_ = {};
};

#endif  // SETTINGS_PAGE_COORDINATOR_H_
