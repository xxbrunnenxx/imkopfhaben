#ifndef PAGE_NAVIGATION_NAVIGATION_MODEL_H_
#define PAGE_NAVIGATION_NAVIGATION_MODEL_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace page_navigation {

enum class NavigationScope : uint8_t {
    kSettings = 0,
    kAdvanced,
    kWifi,
    kDashboard,
    kVibeCheck,
    kSummarize,
    kNotes,
    kTodos,
    kFollowUp,
    kDetails,
    kOnboarding,
};

enum class NavigationItemSection : uint8_t {
    kNone = 0,
    kFooter,
    kSettingsPageMenu,
    kAdvancedPageMenu,
    kWifiPageControls,
    kDashboardPageMenu,
    kVibeCheckPageControls,
    kSummarizePageControls,
    kNotesPageTimelineGroups,
    kTodosPageTimelineGroups,
    kFollowUpPageTimelineGroups,
    kDetailsPageControls,
    kOnboardingPageControls,
};

enum class NavigationItemRole : uint8_t {
    kUnknown = 0,
    kFooterHome,
    kFooterSettings,
    kFooterWifi,
    kFooterSticky,
    kSettingsWifiToggle,
    kSettingsEnableApToggle,
    kSettingsPlaybackToggle,
    kSettingsTimezoneField,
    kSettingsSyncNowButton,
    kSettingsAdvancedButton,
    kAdvancedEnableOtgButton,
    kAdvancedFormatSdButton,
    kAdvancedManualOnboardingButton,
    kAdvancedVolumeSelect,
    kWifiPageNetworkList,
    kWifiPagePasswordInput,
    kWifiPagePasswordVisibilityButton,
    kWifiPageScanButton,
    kWifiPageConnectButton,
    kDashboardMenuItem,
    kVibeCheckPageCard,
    kSummarizePageSegmentControl,
    kSummarizePageScrollContainer,
    kSummarizePageGetSummaryButton,
    kNotesPageTimelineGroup,
    kTodosPageTimelineGroup,
    kFollowUpPageTimelineGroup,
    kDetailsPageScrollContainer,
    kDetailsPageBackButton,
    kDetailsPageTranscribeButton,
    kOnboardingPageClose,
    kOnboardingPagePrev,
    kOnboardingPageNext,
};

struct NavigationItemDescriptor {
    NavigationItemSection section = NavigationItemSection::kNone;
    NavigationItemRole role = NavigationItemRole::kUnknown;
    int item_index = -1;
};

struct NavigationModel {
    NavigationScope scope = NavigationScope::kSettings;
    std::vector<NavigationItemDescriptor> items = {};
    int item_count = 0;

    const NavigationItemDescriptor* ItemAt(int index) const;
    int IndexOfRole(NavigationItemRole role) const;
    bool IsRoleSelected(int selected_index, NavigationItemRole role) const;
};

NavigationModel BuildSettingsPageNavigationModel();
NavigationModel BuildAdvancedPageNavigationModel();
NavigationModel BuildWifiPageNavigationModel();
NavigationModel BuildDashboardPageNavigationModel();
NavigationModel BuildVibeCheckPageNavigationModel();
NavigationModel BuildSummarizePageNavigationModel();
NavigationModel BuildNotesPageNavigationModel(int timeline_group_count);
NavigationModel BuildTodosPageNavigationModel(int timeline_group_count);
NavigationModel BuildFollowUpPageNavigationModel(int timeline_group_count);
// with_transcribe adds a focusable Transcribe button (shown only for audio-only recordings that
// have no transcript yet); when false the page has just the Back button.
NavigationModel BuildDetailsPageNavigationModel(bool with_transcribe = false);
NavigationModel BuildOnboardingPageNavigationModel();

}  // namespace page_navigation

#endif  // PAGE_NAVIGATION_NAVIGATION_MODEL_H_
