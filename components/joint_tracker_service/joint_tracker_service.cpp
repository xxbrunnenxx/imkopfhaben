#include "joint_tracker_service.h"

#include <algorithm>
#include <ctime>
#include <mutex>

#include "esp_log.h"
#include "nvs.h"

namespace joint_tracker_service {
namespace {

constexpr const char* kTag = "joint_tracker";
constexpr const char* kNvsNamespace = "jointtrack";
constexpr const char* kTodayCountKey = "today_cnt";
constexpr const char* kTodayDayKey = "today_day";
constexpr const char* kHistoryKey = "history";

std::mutex s_mutex;
bool s_loaded = false;
int s_today_count = 0;
int32_t s_today_day = 0;              // YYYYMMDD des Zaehlers, 0 = noch keiner
std::vector<DayCount> s_history;      // aelteste zuerst, ohne den heutigen Tag

// Heutiger Kalendertag als YYYYMMDD in Ortszeit. 0, wenn die Uhr noch nicht
// gestellt ist (Zeit vor 2021) -- dann behandeln wir den Tag als unbekannt und
// ruehren den Zaehler nicht an, um keinen falschen Reset auszuloesen.
int32_t HeuteAlsTag()
{
    const time_t now = time(nullptr);
    if (now < 1609459200) {  // 2021-01-01, Schwelle fuer "Uhr gestellt"
        return 0;
    }
    struct tm local_tm = {};
    localtime_r(&now, &local_tm);
    return (local_tm.tm_year + 1900) * 10000 + (local_tm.tm_mon + 1) * 100 + local_tm.tm_mday;
}

void PersistTodayLocked(nvs_handle_t handle)
{
    nvs_set_i32(handle, kTodayCountKey, s_today_count);
    nvs_set_i32(handle, kTodayDayKey, s_today_day);
}

void PersistHistoryLocked(nvs_handle_t handle)
{
    if (s_history.empty()) {
        nvs_erase_key(handle, kHistoryKey);
        return;
    }
    nvs_set_blob(handle, kHistoryKey, s_history.data(), s_history.size() * sizeof(DayCount));
}

// Schreibt den aktuellen Stand (Zaehler + Protokoll) in einem Rutsch nach NVS.
void PersistAllLocked()
{
    nvs_handle_t handle = 0;
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGW(kTag, "nvs_open zum Schreiben fehlgeschlagen");
        return;
    }
    PersistTodayLocked(handle);
    PersistHistoryLocked(handle);
    nvs_commit(handle);
    nvs_close(handle);
}

// Verschiebt den gestrigen Stand ins Protokoll, wenn ein neuer Tag begonnen hat.
// Gibt true zurueck, wenn sich etwas geaendert hat.
bool RolloverLocked()
{
    const int32_t heute = HeuteAlsTag();
    if (heute == 0) {
        return false;  // Uhr nicht gestellt -- nichts tun
    }
    if (s_today_day == 0) {
        // Erster je gezaehlte Tag: einfach uebernehmen, ohne Protokolleintrag.
        s_today_day = heute;
        return false;
    }
    if (s_today_day == heute) {
        return false;  // gleicher Tag, nichts zu tun
    }

    // Tageswechsel: den abgeschlossenen Tag ins Protokoll aufnehmen (auch 0, damit
    // luecken-freie Auswertung moeglich ist), dann Zaehler zuruecksetzen.
    s_history.push_back(DayCount{s_today_day, static_cast<int16_t>(s_today_count)});
    if (static_cast<int>(s_history.size()) > kHistoryDays) {
        s_history.erase(s_history.begin(),
                        s_history.begin() + (s_history.size() - kHistoryDays));
    }
    s_today_day = heute;
    s_today_count = 0;
    return true;
}

void EnsureLoadedLocked()
{
    if (s_loaded) {
        return;
    }
    s_loaded = true;

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        int32_t v = 0;
        if (nvs_get_i32(handle, kTodayCountKey, &v) == ESP_OK) {
            s_today_count = std::max(0, static_cast<int>(v));
        }
        if (nvs_get_i32(handle, kTodayDayKey, &v) == ESP_OK) {
            s_today_day = v;
        }
        size_t blob_size = 0;
        if (nvs_get_blob(handle, kHistoryKey, nullptr, &blob_size) == ESP_OK && blob_size > 0) {
            s_history.resize(blob_size / sizeof(DayCount));
            nvs_get_blob(handle, kHistoryKey, s_history.data(), &blob_size);
        }
        nvs_close(handle);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(kTag, "nvs_open zum Lesen fehlgeschlagen: %s", esp_err_to_name(err));
    }

    ESP_LOGI(kTag, "geladen: heute %d (Tag %d), Protokoll %d Tage",
             s_today_count, static_cast<int>(s_today_day), static_cast<int>(s_history.size()));
}

}  // namespace

void Load()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    if (RolloverLocked()) {
        PersistAllLocked();
    }
}

int GetTodayCount()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    if (RolloverLocked()) {
        PersistAllLocked();
    }
    return s_today_count;
}

int GetDailyGoal()
{
    return kDailyGoal;
}

int Increment()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    RolloverLocked();
    ++s_today_count;
    PersistAllLocked();
    return s_today_count;
}

int Decrement()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    RolloverLocked();
    if (s_today_count > 0) {
        --s_today_count;
    }
    PersistAllLocked();
    return s_today_count;
}

bool RolloverIfNeeded()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    const bool changed = RolloverLocked();
    if (changed) {
        PersistAllLocked();
    }
    return changed;
}

std::vector<DayCount> GetHistory()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    EnsureLoadedLocked();
    RolloverLocked();
    std::vector<DayCount> out = s_history;
    if (s_today_day != 0) {
        out.push_back(DayCount{s_today_day, static_cast<int16_t>(s_today_count)});
    }
    return out;
}

}  // namespace joint_tracker_service
