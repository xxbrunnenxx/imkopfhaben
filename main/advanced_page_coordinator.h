#ifndef ADVANCED_PAGE_COORDINATOR_H_
#define ADVANCED_PAGE_COORDINATOR_H_

#include "epaper_ui/advanced_page.h"
#include "page_navigation/navigation_model.h"
#include "page_navigation/roving_focus.h"
#include "storage_service.h"

class AdvancedPageCoordinator {
public:
    AdvancedPageCoordinator();

    void Show();
    bool MoveFocus(int delta);
    bool SetFocusIndex(int index);
    bool IsRoleFocused(page_navigation::NavigationItemRole role) const;

    // Einstell-Modus fuer die Lautstaerke: ist er aktiv, verstellen Up/Down die
    // Lautstaerke statt den Fokus zu bewegen. Ein Klick schaltet ihn um.
    bool volume_editing() const { return volume_editing_; }
    void SetVolumeEditing(bool editing) { volume_editing_ = editing; }
    void ToggleVolumeEditing() { volume_editing_ = !volume_editing_; }

    epaper_ui::AdvancedPageState BuildState(const storage_service::Snapshot& storage_snapshot) const;

    const page_navigation::NavigationModel& navigation_model() const { return navigation_model_; }
    const page_navigation::RovingFocus& focus() const { return focus_; }

private:
    page_navigation::NavigationModel navigation_model_ =
        page_navigation::BuildAdvancedPageNavigationModel();
    page_navigation::RovingFocus focus_{navigation_model_.item_count, 0};
    bool volume_editing_ = false;
};

#endif  // ADVANCED_PAGE_COORDINATOR_H_
