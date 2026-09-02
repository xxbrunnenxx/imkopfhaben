#ifndef LOCAL_AI_SERVICE_H_
#define LOCAL_AI_SERVICE_H_

#include <cstdint>
#include <string>

#include "esp_err.h"
#include "esp_http_server.h"

namespace recording_service {
class RecordedClip;
}

namespace local_ai_service {

// Where the effective base URL / transcription URL came from. Neither is a secret (the local
// server takes no credential), but we keep the NVS-override vs. built-in-default distinction so
// the portal settings UI can show where a value is coming from, same as the old API-key source.
enum class UrlSource : uint8_t {
    kBuiltIn = 0,
    kNvs,
};

struct SettingsSnapshot {
    bool configured = false;
    bool has_stored_base_url = false;
    bool has_stored_transcribe_url = false;
    UrlSource base_url_source = UrlSource::kBuiltIn;
    std::string base_url;
    std::string transcribe_url;
    std::string model_name;
};

struct RuntimeSnapshot {
    bool initialized = false;
    bool ready = false;
    bool request_in_flight = false;
    bool auth_checked = false;
    bool authenticated = false;
    int last_http_status = 0;
    std::string last_status_message;
    std::string last_model_resource_name;
    std::string last_model_display_name;
    std::string last_error_code;
    std::string last_error_message;
};

struct Snapshot {
    SettingsSnapshot settings = {};
    RuntimeSnapshot runtime = {};
};

struct Event {
    Snapshot snapshot = {};
};

// A patch may update either URL independently; unset fields are left as-is.
struct SettingsPatch {
    bool has_base_url = false;
    std::string base_url;
    bool has_transcribe_url = false;
    std::string transcribe_url;
};

struct Result {
    bool success = false;
    bool validation_error = false;
    int status_code = 500;
    std::string field;
    std::string error_code;
    std::string message;
};

// Result of a synchronous text-generation (chat/completions) call.
struct TextResult {
    bool success = false;
    int http_status = 0;
    std::string text = {};
    std::string error_code = {};
    std::string error_message = {};
};

// Result of a synchronous audio transcription call against the local Whisper endpoint.
struct TranscriptionResult {
    bool success = false;
    int http_status = 0;
    std::string transcript = {};
    std::string error_code = {};
    std::string error_message = {};
    uint32_t clip_duration_ms = 0;
    size_t wav_bytes = 0;
    uint64_t total_elapsed_ms = 0;
};

using EventHandler = void (*)(const Event& event, void* context);

esp_err_t Init();
void SetEventHandler(EventHandler handler, void* context);
Snapshot GetSnapshot();

Result ApplySettingsPatch(const SettingsPatch& patch);
Result ResetStoredSettings();

std::string GetEffectiveBaseUrl();
std::string GetEffectiveTranscribeUrl();
std::string GetEffectiveModelName();
// Synchronous calls against the local backend (block on HTTP; run them from a worker task,
// never a UI/input task).
TextResult GenerateText(const std::string& prompt);
TranscriptionResult Transcribe(const recording_service::RecordedClip& clip);
bool BeginAuthentication();
void SetNetworkState(bool connected, bool access_point_mode);
void RegisterPortalRoutes(httpd_handle_t server);

const char* UrlSourceName(UrlSource source);

}  // namespace local_ai_service

#endif  // LOCAL_AI_SERVICE_H_
