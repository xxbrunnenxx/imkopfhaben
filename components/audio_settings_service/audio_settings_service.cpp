#include "audio_settings_service.h"

#include <algorithm>
#include <mutex>

#include "audio_codec.h"
#include "esp_log.h"
#include "nvs.h"

namespace audio_settings_service {
namespace {

constexpr const char* kTag = "audio_settings";
constexpr const char* kNvsNamespace = "audio";
constexpr const char* kVolumeKey = "out_vol";

std::mutex s_mutex;
int s_volume_index = -1;  // -1 = noch nicht geladen

// Klemmt einen rohen Prozentwert auf die naechstgelegene bekannte Stufe.
int IndexForVolume(int volume)
{
    int best_index = 0;
    int best_distance = 1000;
    for (int i = 0; i < kVolumeStepCount; ++i) {
        const int distance = std::abs(kVolumeSteps[i] - volume);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    return best_index;
}

int DefaultIndex()
{
    return IndexForVolume(kDefaultVolume);
}

void PersistLocked(int index)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "nvs_open zum Schreiben fehlgeschlagen: %s", esp_err_to_name(err));
        return;
    }
    err = nvs_set_u8(handle, kVolumeKey, static_cast<uint8_t>(kVolumeSteps[index]));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "Lautstaerke speichern fehlgeschlagen: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
}

}  // namespace

void Load()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    int index = DefaultIndex();
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kNvsNamespace, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        uint8_t stored = 0;
        if (nvs_get_u8(handle, kVolumeKey, &stored) == ESP_OK) {
            index = IndexForVolume(static_cast<int>(stored));
        }
        nvs_close(handle);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(kTag, "nvs_open zum Lesen fehlgeschlagen: %s", esp_err_to_name(err));
    }

    s_volume_index = index;
    ESP_LOGI(kTag, "Lautstaerke geladen: %d %%", kVolumeSteps[index]);
}

int GetVolumeIndex()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_volume_index < 0) {
        s_volume_index = DefaultIndex();
    }
    return s_volume_index;
}

int GetVolume()
{
    return kVolumeSteps[GetVolumeIndex()];
}

bool SetVolumeIndex(int index)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const int clamped = std::clamp(index, 0, kVolumeStepCount - 1);
    if (s_volume_index < 0) {
        s_volume_index = DefaultIndex();
    }
    if (clamped == s_volume_index) {
        return false;
    }
    s_volume_index = clamped;
    PersistLocked(clamped);
    ESP_LOGI(kTag, "Lautstaerke gesetzt: %d %%", kVolumeSteps[clamped]);
    return true;
}

void ApplyVolumeToCodec(AudioCodec* codec)
{
    if (codec == nullptr) {
        return;
    }
    const int volume = GetVolume();
    // Erst die Zahl setzen, dann Mute entsprechend nachziehen: bei 0 % soll der
    // Ausgang wirklich still sein, sonst offen.
    codec->SetOutputVolume(volume);
    codec->SetOutputMuted(volume == 0);
}

}  // namespace audio_settings_service
