#include "page_navigation/navigation_model.h"

namespace page_navigation {
namespace {

void AddItem(NavigationModel& model,
             NavigationItemSection section,
             NavigationItemRole role,
             int item_index)
{
    model.items.push_back({
        .section = section,
        .role = role,
        .item_index = item_index,
    });
    model.item_count = static_cast<int>(model.items.size());
}

// Every interactive page (all but onboarding) ends its navigation model with the same four
// footer icons at the same item_index values, since the footer itself is shared/global.
void AddFooterNavigationItems(NavigationModel& model)
{
    AddItem(model, NavigationItemSection::kFooter, NavigationItemRole::kFooterSettings, 1);
    AddItem(model, NavigationItemSection::kFooter, NavigationItemRole::kFooterWifi, 2);
    AddItem(model, NavigationItemSection::kFooter, NavigationItemRole::kFooterSticky, 3);
    AddItem(model, NavigationItemSection::kFooter, NavigationItemRole::kFooterHome, 0);
}

}  // namespace

const NavigationItemDescriptor* NavigationModel::ItemAt(int index) const
{
    if (index < 0 || index >= item_count) {
        return nullptr;
    }
    return &items[static_cast<size_t>(index)];
}

int NavigationModel::IndexOfRole(NavigationItemRole role) const
{
    for (int index = 0; index < item_count; ++index) {
        if (items[static_cast<size_t>(index)].role == role) {
            return index;
        }
    }
    return -1;
}

bool NavigationModel::IsRoleSelected(int selected_index, NavigationItemRole role) const
{
    const NavigationItemDescriptor* item = ItemAt(selected_index);
    return item != nullptr && item->role == role;
}

NavigationModel BuildSettingsPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kSettings;

    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsWifiToggle,
            0);
    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsEnableApToggle,
            1);
    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsPlaybackToggle,
            2);
    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsTimezoneField,
            3);
    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsSyncNowButton,
            4);
    AddItem(model,
            NavigationItemSection::kSettingsPageMenu,
            NavigationItemRole::kSettingsAdvancedButton,
            5);
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildAdvancedPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kAdvanced;

    AddItem(model,
            NavigationItemSection::kAdvancedPageMenu,
            NavigationItemRole::kAdvancedEnableOtgButton,
            0);
    AddItem(model,
            NavigationItemSection::kAdvancedPageMenu,
            NavigationItemRole::kAdvancedFormatSdButton,
            1);
    AddItem(model,
            NavigationItemSection::kAdvancedPageMenu,
            NavigationItemRole::kAdvancedManualOnboardingButton,
            2);
    AddItem(model,
            NavigationItemSection::kAdvancedPageMenu,
            NavigationItemRole::kAdvancedVolumeSelect,
            3);
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildWifiPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kWifi;

    AddItem(model,
            NavigationItemSection::kWifiPageControls,
            NavigationItemRole::kWifiPageNetworkList,
            0);
    AddItem(model,
            NavigationItemSection::kWifiPageControls,
            NavigationItemRole::kWifiPagePasswordInput,
            1);
    AddItem(model,
            NavigationItemSection::kWifiPageControls,
            NavigationItemRole::kWifiPagePasswordVisibilityButton,
            2);
    AddItem(model,
            NavigationItemSection::kWifiPageControls,
            NavigationItemRole::kWifiPageScanButton,
            3);
    AddItem(model,
            NavigationItemSection::kWifiPageControls,
            NavigationItemRole::kWifiPageConnectButton,
            4);
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildDashboardPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kDashboard;

    for (int index = 0; index < 5; ++index) {
        AddItem(model, NavigationItemSection::kDashboardPageMenu,
                NavigationItemRole::kDashboardMenuItem, index);
    }
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildVibeCheckPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kVibeCheck;

    AddItem(model, NavigationItemSection::kVibeCheckPageControls,
            NavigationItemRole::kVibeCheckPageCard, 0);
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildSummarizePageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kSummarize;

    AddItem(model, NavigationItemSection::kSummarizePageControls,
            NavigationItemRole::kSummarizePageSegmentControl, 0);
    AddItem(model, NavigationItemSection::kSummarizePageControls,
            NavigationItemRole::kSummarizePageScrollContainer, 1);
    AddItem(model, NavigationItemSection::kSummarizePageControls,
            NavigationItemRole::kSummarizePageGetSummaryButton, 2);
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildNotesPageNavigationModel(int timeline_group_count)
{
    NavigationModel model = {};
    model.scope = NavigationScope::kNotes;

    const int group_count = timeline_group_count > 0 ? timeline_group_count : 0;
    for (int index = 0; index < group_count; ++index) {
        AddItem(model, NavigationItemSection::kNotesPageTimelineGroups,
                NavigationItemRole::kNotesPageTimelineGroup, index);
    }
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildTodosPageNavigationModel(int timeline_group_count)
{
    NavigationModel model = {};
    model.scope = NavigationScope::kTodos;

    const int group_count = timeline_group_count > 0 ? timeline_group_count : 0;
    for (int index = 0; index < group_count; ++index) {
        AddItem(model, NavigationItemSection::kTodosPageTimelineGroups,
                NavigationItemRole::kTodosPageTimelineGroup, index);
    }
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildFollowUpPageNavigationModel(int timeline_group_count)
{
    NavigationModel model = {};
    model.scope = NavigationScope::kFollowUp;

    const int group_count = timeline_group_count > 0 ? timeline_group_count : 0;
    for (int index = 0; index < group_count; ++index) {
        AddItem(model, NavigationItemSection::kFollowUpPageTimelineGroups,
                NavigationItemRole::kFollowUpPageTimelineGroup, index);
    }
    AddFooterNavigationItems(model);
    return model;
}

NavigationModel BuildOnboardingPageNavigationModel()
{
    NavigationModel model = {};
    model.scope = NavigationScope::kOnboarding;

    // The carousel's own control row replaces the footer: Close, Prev, Next (visual left -> right).
    AddItem(model, NavigationItemSection::kOnboardingPageControls,
            NavigationItemRole::kOnboardingPageClose, 0);
    AddItem(model, NavigationItemSection::kOnboardingPageControls,
            NavigationItemRole::kOnboardingPagePrev, 1);
    AddItem(model, NavigationItemSection::kOnboardingPageControls,
            NavigationItemRole::kOnboardingPageNext, 2);
    return model;
}

NavigationModel BuildDetailsPageNavigationModel(bool with_transcribe)
{
    NavigationModel model = {};
    model.scope = NavigationScope::kDetails;

    AddItem(model, NavigationItemSection::kDetailsPageControls,
            NavigationItemRole::kDetailsPageScrollContainer, 0);
    AddItem(model, NavigationItemSection::kDetailsPageControls,
            NavigationItemRole::kDetailsPageBackButton, 1);
    // Audio-only recordings (no transcript yet) gain a Transcribe button to the right of Back.
    if (with_transcribe) {
        AddItem(model, NavigationItemSection::kDetailsPageControls,
                NavigationItemRole::kDetailsPageTranscribeButton, 2);
    }
    AddFooterNavigationItems(model);
    return model;
}

}  // namespace page_navigation
