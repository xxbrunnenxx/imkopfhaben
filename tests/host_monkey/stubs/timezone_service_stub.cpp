// Host-Stub fuer timezone_service. timeline_format.cpp fragt nur
// GetSnapshot().runtime.current_date ab, um zu entscheiden, ob ein Datum als
// "Today" beschriftet wird. Der Test setzt ein festes heutiges Datum, damit
// die Gruppierung deterministisch ist.
#include "timezone_service.h"

namespace timezone_service {

static std::string s_today = "2026-09-21";

Snapshot GetSnapshot()
{
    Snapshot snap = {};
    snap.runtime.current_date = s_today;
    snap.runtime.time_valid = true;
    return snap;
}

// Der Rest der API wird vom Coordinator/timeline_format nicht gebraucht,
// muss aber fuer den Linker existieren, falls doch referenziert.
esp_err_t Init() { return ESP_OK; }
void SetEventHandler(EventHandler, void*) {}
std::vector<TimezoneInfo> ListTimezones() { return {}; }
void SetNetworkConnected(bool) {}
Result ApplySettingsPatch(const SettingsPatch&) { return {}; }
bool SyncNow(const char*, uint32_t) { return false; }
bool IsSyncInProgress() { return false; }
void RegisterPortalRoutes(httpd_handle_t) {}

}  // namespace timezone_service
