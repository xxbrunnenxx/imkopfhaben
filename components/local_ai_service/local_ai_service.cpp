#include "local_ai_service.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <strings.h>
#include <utility>

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "followup_task_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "recording_service.h"
#include "sdkconfig.h"

// Local AI backend: a household server ("Kraken") running LM Studio's OpenAI-compatible REST
// API for text generation (google/gemma-4-e2b) and a small local endpoint wrapping
// faster-whisper for transcription. Both are plain HTTP on the LAN -- no TLS, no API key. See
// docs/local-ai-service.md for the facts this was built against (verified 2026-09-01) and what
// is still open.
namespace local_ai_service {
namespace {

constexpr const char* kTag = "LocalAiService";
constexpr const char* kSettingsTag = "LocalAiSettings";
constexpr const char* kStorageNamespace = "local_ai";
constexpr const char* kStorageBaseUrl = "base_url";
constexpr const char* kStorageTranscribeUrl = "transcribe_url";
// Kconfig-backed like the URLs below, not NVS/portal-overridable -- switching model or
// reasoning_effort for a different local model is a firmware-config change, not a runtime one.
#if defined(CONFIG_FOLLOWUP_LOCAL_AI_MODEL_NAME)
constexpr const char* kDefaultModelName = CONFIG_FOLLOWUP_LOCAL_AI_MODEL_NAME;
#else
constexpr const char* kDefaultModelName = "google/gemma-4-e2b";
#endif
constexpr const char* kPortalApiSettingsUri = "/api/settings/local_ai";
constexpr const char* kPortalApiSettingsResetUri = "/api/settings/local_ai/reset";
constexpr const char* kPortalApiRuntimeUri = "/api/runtime/local_ai";
constexpr size_t kMaxPortalPayloadLen = 512;
constexpr int kAuthTimeoutMs = 5000;      // LAN round-trip, not a WAN one -- keep this tight
constexpr int kGenerateTimeoutMs = 60000;  // reasoning_effort=none keeps this well under budget
                                            // in practice, but leave headroom for a cold model
constexpr int kTranscribeTimeoutMs = 30000;
constexpr uint32_t kAuthTaskStackWords = 8192;

// Mandatory for the default model: without this, gemma-4-e2b spends most of its output budget
// on hidden reasoning_content before it ever emits a real answer. Measured: a trivial
// one-sentence prompt used 122 completion tokens, 114 of them reasoning, until this flag was
// added. See docs/local-ai-service.md. Kconfig-backed (see kDefaultModelName above) so pointing
// this firmware at a different local model doesn't require a source change here.
#if defined(CONFIG_FOLLOWUP_LOCAL_AI_REASONING_EFFORT)
constexpr const char* kReasoningEffortNone = CONFIG_FOLLOWUP_LOCAL_AI_REASONING_EFFORT;
#else
constexpr const char* kReasoningEffortNone = "none";
#endif

struct AuthResult {
    bool success = false;
    int http_status = 0;
    std::string model_resource_name;
    std::string model_display_name;
    std::string error_code;
    std::string error_message;
};

struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::string error_code;
    std::string error_message;
};

struct AuthTaskContext {
    std::string base_url;
    std::string model_name;
    uint32_t generation = 0;
};

std::mutex s_mutex;
EventHandler s_event_handler = nullptr;
void* s_event_context = nullptr;
bool s_initialized = false;
bool s_network_connected = false;
bool s_access_point_mode = false;
bool s_request_in_flight = false;
bool s_auth_checked = false;
bool s_authenticated = false;
uint32_t s_auth_generation = 0;
int s_last_http_status = 0;
std::string s_stored_base_url;
std::string s_stored_transcribe_url;
std::string s_last_status_message;
std::string s_last_model_resource_name;
std::string s_last_model_display_name;
std::string s_last_error_code;
std::string s_last_error_message;

std::string TrimCopy(std::string value)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string TrimForLog(std::string value, size_t max_len = 96)
{
    value = TrimCopy(std::move(value));
    if (value.size() <= max_len) {
        return value;
    }
    if (max_len <= 3) {
        return value.substr(0, max_len);
    }
    return value.substr(0, max_len - 3) + "...";
}

// Ensure a base URL ends with a single trailing slash, so callers can just append e.g. "models".
std::string NormalizeBaseUrl(std::string url)
{
    url = TrimCopy(std::move(url));
    if (!url.empty() && url.back() != '/') {
        url += '/';
    }
    return url;
}

std::string ReadNvsString(nvs_handle_t handle, const char* key)
{
    size_t size = 0;
    if (nvs_get_str(handle, key, nullptr, &size) != ESP_OK || size == 0) {
        return {};
    }

    std::string value(size, '\0');
    if (nvs_get_str(handle, key, value.data(), &size) != ESP_OK) {
        return {};
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

bool WriteNvsString(nvs_handle_t handle, const char* key, const std::string& value)
{
    esp_err_t err = value.empty() ? nvs_erase_key(handle, key) : nvs_set_str(handle, key, value.c_str());
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = ESP_OK;  // erasing a key that was never set is not a failure
    }
    return err == ESP_OK;
}

struct StoredUrls {
    std::string base_url;
    std::string transcribe_url;
};

StoredUrls LoadStoredUrls()
{
    StoredUrls stored = {};
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kStorageNamespace, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return stored;
    }
    if (err != ESP_OK) {
        ESP_LOGW(kSettingsTag, "Failed to open local_ai NVS namespace: %s", esp_err_to_name(err));
        return stored;
    }

    stored.base_url = NormalizeBaseUrl(ReadNvsString(handle, kStorageBaseUrl));
    stored.transcribe_url = TrimCopy(ReadNvsString(handle, kStorageTranscribeUrl));
    nvs_close(handle);
    return stored;
}

bool SaveStoredUrls(const StoredUrls& stored)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kStorageNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(kSettingsTag, "Failed to open local_ai NVS namespace for write: %s",
                 esp_err_to_name(err));
        return false;
    }

    bool ok = WriteNvsString(handle, kStorageBaseUrl, stored.base_url) &&
              WriteNvsString(handle, kStorageTranscribeUrl, stored.transcribe_url);
    if (ok) {
        ok = nvs_commit(handle) == ESP_OK;
    }
    nvs_close(handle);

    if (!ok) {
        ESP_LOGE(kSettingsTag, "Failed to save local AI settings");
    }
    return ok;
}

bool ClearStoredUrlsFromNvs()
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kStorageNamespace, NVS_READWRITE, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(kSettingsTag, "Failed to open local_ai NVS namespace for clear: %s",
                 esp_err_to_name(err));
        return false;
    }

    esp_err_t erase_base = nvs_erase_key(handle, kStorageBaseUrl);
    esp_err_t erase_transcribe = nvs_erase_key(handle, kStorageTranscribeUrl);
    bool ok = (erase_base == ESP_OK || erase_base == ESP_ERR_NVS_NOT_FOUND) &&
              (erase_transcribe == ESP_OK || erase_transcribe == ESP_ERR_NVS_NOT_FOUND);
    if (ok) {
        ok = nvs_commit(handle) == ESP_OK;
    }
    nvs_close(handle);

    if (!ok) {
        ESP_LOGE(kSettingsTag, "Failed to clear local AI settings");
    }
    return ok;
}

std::string GetSdkConfigBaseUrl()
{
#if defined(CONFIG_FOLLOWUP_LOCAL_AI_BASE_URL)
    return NormalizeBaseUrl(CONFIG_FOLLOWUP_LOCAL_AI_BASE_URL);
#else
    return {};
#endif
}

std::string GetSdkConfigTranscribeUrl()
{
#if defined(CONFIG_FOLLOWUP_LOCAL_AI_TRANSCRIBE_URL)
    return TrimCopy(CONFIG_FOLLOWUP_LOCAL_AI_TRANSCRIBE_URL);
#else
    return {};
#endif
}

std::string GetEffectiveBaseUrlLocked()
{
    if (!s_stored_base_url.empty()) {
        return s_stored_base_url;
    }
    return GetSdkConfigBaseUrl();
}

std::string GetEffectiveTranscribeUrlLocked()
{
    if (!s_stored_transcribe_url.empty()) {
        return s_stored_transcribe_url;
    }
    return GetSdkConfigTranscribeUrl();
}

UrlSource GetUrlSourceLocked()
{
    return s_stored_base_url.empty() ? UrlSource::kBuiltIn : UrlSource::kNvs;
}

void SetLastErrorLocked(const char* error_code, const char* message)
{
    s_last_error_code = error_code != nullptr ? error_code : "";
    s_last_error_message = message != nullptr ? message : "";
}

void ClearLastErrorLocked()
{
    s_last_error_code.clear();
    s_last_error_message.clear();
}

Snapshot BuildSnapshotLocked()
{
    const std::string base_url = GetEffectiveBaseUrlLocked();
    const bool configured = !base_url.empty();

    Snapshot snapshot = {};
    snapshot.settings.configured = configured;
    snapshot.settings.has_stored_base_url = !s_stored_base_url.empty();
    snapshot.settings.has_stored_transcribe_url = !s_stored_transcribe_url.empty();
    snapshot.settings.base_url_source = GetUrlSourceLocked();
    snapshot.settings.base_url = base_url;
    snapshot.settings.transcribe_url = GetEffectiveTranscribeUrlLocked();
    snapshot.settings.model_name = kDefaultModelName;

    snapshot.runtime.initialized = s_initialized;
    snapshot.runtime.ready = configured && s_authenticated;
    snapshot.runtime.request_in_flight = s_request_in_flight;
    snapshot.runtime.auth_checked = s_auth_checked;
    snapshot.runtime.authenticated = s_authenticated;
    snapshot.runtime.supports_audio_understanding = false;
    snapshot.runtime.supports_structured_output = false;
    snapshot.runtime.last_http_status = s_last_http_status;
    snapshot.runtime.last_status_message = s_last_status_message;
    snapshot.runtime.last_model_resource_name = s_last_model_resource_name;
    snapshot.runtime.last_model_display_name = s_last_model_display_name;
    snapshot.runtime.last_error_code = s_last_error_code;
    snapshot.runtime.last_error_message = s_last_error_message;
    return snapshot;
}

void Notify()
{
    EventHandler handler = nullptr;
    void* context = nullptr;
    Event event = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        handler = s_event_handler;
        context = s_event_context;
        event.snapshot = BuildSnapshotLocked();
    }

    if (handler != nullptr) {
        handler(event, context);
    }
}

bool ShouldStartAuthenticationLocked()
{
    return s_initialized &&
           s_network_connected &&
           !s_request_in_flight &&
           !s_authenticated &&
           !GetEffectiveBaseUrlLocked().empty();
}

esp_err_t HttpEventHandler(esp_http_client_event_t* event)
{
    if (event == nullptr) {
        return ESP_FAIL;
    }
    auto* response = static_cast<HttpResponse*>(event->user_data);
    if (event->event_id == HTTP_EVENT_ON_DATA &&
        response != nullptr &&
        event->data != nullptr &&
        event->data_len > 0) {
        response->body.append(static_cast<const char*>(event->data),
                              static_cast<size_t>(event->data_len));
    }
    return ESP_OK;
}

std::string JsonStringField(cJSON* root, const char* key)
{
    if (root == nullptr || key == nullptr) {
        return {};
    }

    cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(item) || item->valuestring == nullptr) {
        return {};
    }
    return item->valuestring;
}

void PopulateHttpError(cJSON* root, const HttpResponse& response,
                       std::string* error_code, std::string* error_message)
{
    if (error_code == nullptr || error_message == nullptr) {
        return;
    }

    *error_code = "http_error";
    error_message->clear();
    if (root != nullptr) {
        cJSON* error = cJSON_GetObjectItemCaseSensitive(root, "error");
        if (cJSON_IsObject(error)) {
            const std::string message = JsonStringField(error, "message");
            if (!message.empty()) {
                *error_message = message;
            }
        } else if (cJSON_IsString(error) && error->valuestring != nullptr) {
            *error_message = error->valuestring;
        }
    }
    if (error_message->empty()) {
        *error_message =
            response.body.empty() ? "Local AI request failed" : response.body;
    }
    *error_message = TrimForLog(std::move(*error_message));
}

HttpResponse PerformGet(const std::string& url, int timeout_ms)
{
    HttpResponse response = {};

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = timeout_ms;
    config.event_handler = &HttpEventHandler;
    config.user_data = &response;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        response.error_code = "http_client_init_failed";
        response.error_message = "Failed to initialize local AI HTTP client";
        return response;
    }

    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "User-Agent", "folloup-sticky");

    const esp_err_t err = esp_http_client_perform(client);
    response.status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        response.error_code = "transport_error";
        response.error_message = esp_err_to_name(err);
    }
    return response;
}

// Synchronous JSON POST (chat/completions).
HttpResponse PerformJsonPost(const std::string& url, const std::string& body, int timeout_ms)
{
    HttpResponse response = {};

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = timeout_ms;
    config.event_handler = &HttpEventHandler;
    config.user_data = &response;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        response.error_code = "http_client_init_failed";
        response.error_message = "Failed to initialize local AI HTTP client";
        return response;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "User-Agent", "folloup-sticky");
    esp_http_client_set_post_field(client, body.c_str(), static_cast<int>(body.size()));

    const esp_err_t err = esp_http_client_perform(client);
    response.status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        response.error_code = "transport_error";
        response.error_message = esp_err_to_name(err);
    }
    return response;
}

// Matches whether the /v1/models "data" array contains an entry whose "id" equals model_name;
// returns that id as both resource + display name (LM Studio's model list has no separate
// display name field).
bool FindModelInModelsResponse(cJSON* root, const std::string& model_name, std::string* found_id)
{
    if (root == nullptr) {
        return false;
    }
    cJSON* data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!cJSON_IsArray(data)) {
        return false;
    }
    cJSON* entry = nullptr;
    cJSON_ArrayForEach(entry, data)
    {
        const std::string id = JsonStringField(entry, "id");
        if (id == model_name) {
            if (found_id != nullptr) {
                *found_id = id;
            }
            return true;
        }
    }
    return false;
}

AuthResult Authenticate(const std::string& base_url, const std::string& model_name)
{
    AuthResult result = {};
    const HttpResponse http = PerformGet(base_url + "models", kAuthTimeoutMs);
    result.http_status = http.status_code;

    if (!http.error_code.empty()) {
        result.error_code = http.error_code;
        result.error_message = TrimForLog(http.error_message);
        return result;
    }

    cJSON* root = cJSON_ParseWithLength(http.body.c_str(), http.body.size());
    if (http.status_code >= 200 && http.status_code < 300) {
        std::string found_id;
        const bool model_listed = FindModelInModelsResponse(root, model_name, &found_id);
        result.success = model_listed;
        result.model_resource_name = model_listed ? found_id : model_name;
        result.model_display_name = model_listed ? found_id : (model_name + " (not listed)");
        if (!model_listed) {
            result.error_code = "model_not_listed";
            result.error_message = "Configured model \"" + model_name + "\" not found in /v1/models response";
        }
        if (root != nullptr) {
            cJSON_Delete(root);
        }
        return result;
    }

    PopulateHttpError(root, http, &result.error_code, &result.error_message);
    if (root != nullptr) {
        cJSON_Delete(root);
    }
    return result;
}

void CompleteAuthentication(uint32_t generation, const AuthResult& result)
{
    bool stale_result = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (generation != s_auth_generation) {
            stale_result = true;
        }
        if (!stale_result) {
            s_request_in_flight = false;
            s_auth_checked = true;
            s_last_http_status = result.http_status;

            if (!result.success) {
                s_authenticated = false;
                s_last_status_message = result.error_code == "model_not_listed"
                                             ? "Local AI server reachable, but configured model not loaded"
                                             : "Local AI server unreachable";
                s_last_model_resource_name.clear();
                s_last_model_display_name.clear();
                SetLastErrorLocked(result.error_code.c_str(), result.error_message.c_str());
            } else {
                s_authenticated = true;
                s_last_model_resource_name = result.model_resource_name;
                s_last_model_display_name = result.model_display_name;
                s_last_status_message = !s_last_model_display_name.empty()
                                            ? "Connected to " + s_last_model_display_name
                                            : "Connected to local AI server";
                ClearLastErrorLocked();
            }
        }
    }

    if (stale_result) {
        ESP_LOGI(kTag, "Ignoring stale local AI readiness result for generation %lu",
                 static_cast<unsigned long>(generation));
        return;
    }

    if (!result.success) {
        ESP_LOGW(kTag, "Local AI readiness check failed: http=%d code=%s message=%s",
                 result.http_status,
                 result.error_code.empty() ? "http_error" : result.error_code.c_str(),
                 result.error_message.empty() ? "unknown" : result.error_message.c_str());
    } else {
        ESP_LOGI(kTag, "Local AI readiness check succeeded: model=%s http=%d",
                 result.model_resource_name.empty() ? "unknown"
                                                    : result.model_resource_name.c_str(),
                 result.http_status);
    }

    Notify();
}

void AuthenticationTask(void* arg)
{
    std::unique_ptr<AuthTaskContext> context(static_cast<AuthTaskContext*>(arg));
    if (context == nullptr) {
        CompleteAuthentication(0, AuthResult{
            .success = false,
            .http_status = 0,
            .model_resource_name = {},
            .model_display_name = {},
            .error_code = "task_context_missing",
            .error_message = "Local AI readiness task context missing",
        });
        vTaskDelete(nullptr);
        return;
    }

    const AuthResult result = Authenticate(context->base_url, context->model_name);
    CompleteAuthentication(context->generation, result);
    vTaskDelete(nullptr);
}

void MaybeBeginAuthentication()
{
    bool should_start = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        should_start = ShouldStartAuthenticationLocked();
    }
    if (should_start) {
        (void)BeginAuthentication();
    }
}

std::string ReadRequestBody(httpd_req_t* request)
{
    if (request == nullptr || request->content_len <= 0) {
        return {};
    }

    std::string body(static_cast<size_t>(request->content_len), '\0');
    size_t offset = 0;
    while (offset < body.size()) {
        const int received = httpd_req_recv(request, body.data() + offset, body.size() - offset);
        if (received <= 0) {
            return {};
        }
        offset += static_cast<size_t>(received);
    }
    return body;
}

std::string JsonString(cJSON* root)
{
    if (root == nullptr) {
        return "{}";
    }

    char* raw = cJSON_PrintUnformatted(root);
    if (raw == nullptr) {
        return "{}";
    }
    std::string json(raw);
    cJSON_free(raw);
    return json;
}

esp_err_t SendJsonResponse(httpd_req_t* request, int status_code, cJSON* root)
{
    if (request == nullptr) {
        if (root != nullptr) {
            cJSON_Delete(root);
        }
        return ESP_FAIL;
    }

    const std::string payload = JsonString(root);
    if (root != nullptr) {
        cJSON_Delete(root);
    }

    switch (status_code) {
        case 200:
            httpd_resp_set_status(request, HTTPD_200);
            break;
        case 400:
            httpd_resp_set_status(request, HTTPD_400);
            break;
        case 500:
        default:
            httpd_resp_set_status(request, HTTPD_500);
            break;
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    return httpd_resp_send(request, payload.c_str(), payload.size());
}

void AppendSnapshot(cJSON* root, const Snapshot& snapshot, const char* message)
{
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "message", message != nullptr ? message : "");

    cJSON* settings = cJSON_AddObjectToObject(root, "settings");
    cJSON_AddBoolToObject(settings, "configured", snapshot.settings.configured);
    cJSON_AddBoolToObject(settings, "has_stored_base_url", snapshot.settings.has_stored_base_url);
    cJSON_AddBoolToObject(settings, "has_stored_transcribe_url",
                          snapshot.settings.has_stored_transcribe_url);
    cJSON_AddStringToObject(settings, "base_url_source",
                            UrlSourceName(snapshot.settings.base_url_source));
    cJSON_AddStringToObject(settings, "base_url", snapshot.settings.base_url.c_str());
    cJSON_AddStringToObject(settings, "transcribe_url", snapshot.settings.transcribe_url.c_str());
    cJSON_AddStringToObject(settings, "model_name", snapshot.settings.model_name.c_str());

    cJSON* runtime = cJSON_AddObjectToObject(root, "runtime");
    cJSON_AddBoolToObject(runtime, "initialized", snapshot.runtime.initialized);
    cJSON_AddBoolToObject(runtime, "ready", snapshot.runtime.ready);
    cJSON_AddBoolToObject(runtime, "request_in_flight", snapshot.runtime.request_in_flight);
    cJSON_AddBoolToObject(runtime, "auth_checked", snapshot.runtime.auth_checked);
    cJSON_AddBoolToObject(runtime, "authenticated", snapshot.runtime.authenticated);
    cJSON_AddBoolToObject(runtime, "supports_audio_understanding",
                          snapshot.runtime.supports_audio_understanding);
    cJSON_AddBoolToObject(runtime, "supports_structured_output",
                          snapshot.runtime.supports_structured_output);
    cJSON_AddNumberToObject(runtime, "last_http_status", snapshot.runtime.last_http_status);
    cJSON_AddStringToObject(runtime, "last_status_message",
                            snapshot.runtime.last_status_message.c_str());
    cJSON_AddStringToObject(runtime, "last_model_resource_name",
                            snapshot.runtime.last_model_resource_name.c_str());
    cJSON_AddStringToObject(runtime, "last_model_display_name",
                            snapshot.runtime.last_model_display_name.c_str());
    cJSON_AddStringToObject(runtime, "last_error_code",
                            snapshot.runtime.last_error_code.c_str());
    cJSON_AddStringToObject(runtime, "last_error_message",
                            snapshot.runtime.last_error_message.c_str());
}

bool ParsePatchBody(const std::string& body, SettingsPatch* patch, std::string* error)
{
    if (patch == nullptr) {
        return false;
    }

    cJSON* root = cJSON_ParseWithLength(body.c_str(), body.size());
    if (root == nullptr) {
        if (error != nullptr) {
            *error = "Invalid JSON body";
        }
        return false;
    }

    cJSON* base_url = cJSON_GetObjectItemCaseSensitive(root, "base_url");
    if (cJSON_IsString(base_url) && base_url->valuestring != nullptr) {
        patch->has_base_url = true;
        patch->base_url = base_url->valuestring;
    } else if (base_url != nullptr && !cJSON_IsNull(base_url)) {
        if (error != nullptr) {
            *error = "Invalid base_url";
        }
        cJSON_Delete(root);
        return false;
    }

    cJSON* transcribe_url = cJSON_GetObjectItemCaseSensitive(root, "transcribe_url");
    if (cJSON_IsString(transcribe_url) && transcribe_url->valuestring != nullptr) {
        patch->has_transcribe_url = true;
        patch->transcribe_url = transcribe_url->valuestring;
    } else if (transcribe_url != nullptr && !cJSON_IsNull(transcribe_url)) {
        if (error != nullptr) {
            *error = "Invalid transcribe_url";
        }
        cJSON_Delete(root);
        return false;
    }

    cJSON_Delete(root);
    return true;
}

esp_err_t RegisterPortalRoute(httpd_handle_t server, const httpd_uri_t* handler)
{
    if (server == nullptr || handler == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_err_t err = httpd_register_uri_handler(server, handler);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to register local AI portal route %s [%d]: %s",
                 handler->uri != nullptr ? handler->uri : "<null>",
                 static_cast<int>(handler->method),
                 esp_err_to_name(err));
    }
    return err;
}

esp_err_t HandlePortalSettingsGet(httpd_req_t* request)
{
    cJSON* root = cJSON_CreateObject();
    AppendSnapshot(root, GetSnapshot(), "Local AI settings loaded");
    return SendJsonResponse(request, 200, root);
}

esp_err_t HandlePortalSettingsPatch(httpd_req_t* request)
{
    if (request == nullptr ||
        request->content_len <= 0 ||
        request->content_len > static_cast<int>(kMaxPortalPayloadLen)) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "message", "Invalid local AI settings payload");
        return SendJsonResponse(request, 400, root);
    }

    const std::string body = ReadRequestBody(request);
    SettingsPatch patch = {};
    std::string parse_error;
    if (body.empty() || !ParsePatchBody(body, &patch, &parse_error)) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "message",
                                parse_error.empty() ? "Invalid local AI settings payload"
                                                    : parse_error.c_str());
        return SendJsonResponse(request, 400, root);
    }

    const Result result = ApplySettingsPatch(patch);
    if (!result.success) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "message", result.message.c_str());
        cJSON_AddStringToObject(root, "error_code", result.error_code.c_str());
        cJSON_AddStringToObject(root, "field", result.field.c_str());
        return SendJsonResponse(request, result.status_code, root);
    }

    cJSON* root = cJSON_CreateObject();
    AppendSnapshot(root, GetSnapshot(), "Local AI settings stored");
    return SendJsonResponse(request, 200, root);
}

esp_err_t HandlePortalSettingsReset(httpd_req_t* request)
{
    const Result result = ResetStoredSettings();
    if (!result.success) {
        cJSON* root = cJSON_CreateObject();
        cJSON_AddBoolToObject(root, "success", false);
        cJSON_AddStringToObject(root, "message", result.message.c_str());
        cJSON_AddStringToObject(root, "error_code", result.error_code.c_str());
        cJSON_AddStringToObject(root, "field", result.field.c_str());
        return SendJsonResponse(request, result.status_code, root);
    }

    cJSON* root = cJSON_CreateObject();
    AppendSnapshot(root, GetSnapshot(), "Local AI settings reset to built-in defaults");
    return SendJsonResponse(request, 200, root);
}

esp_err_t HandlePortalRuntimeGet(httpd_req_t* request)
{
    cJSON* root = cJSON_CreateObject();
    AppendSnapshot(root, GetSnapshot(), "Local AI runtime loaded");
    return SendJsonResponse(request, 200, root);
}

// A single text-part chat message: {"model":..,"messages":[{"role":"user","content":prompt}],
// "temperature":0,"reasoning_effort":"none"}.
std::string BuildChatCompletionRequestBody(const std::string& model_name, const std::string& prompt)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", model_name.c_str());
    cJSON* messages = cJSON_AddArrayToObject(root, "messages");
    cJSON* message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", prompt.c_str());
    cJSON_AddItemToArray(messages, message);
    cJSON_AddNumberToObject(root, "temperature", 0);
    // Mandatory -- see kReasoningEffortNone above.
    cJSON_AddStringToObject(root, "reasoning_effort", kReasoningEffortNone);

    const std::string body = JsonString(root);
    cJSON_Delete(root);
    return body;
}

// choices[0].message.content -- NOT reasoning_content, which carries the (suppressed, but the
// field may still be present and empty) chain-of-thought.
std::string ExtractChatCompletionText(cJSON* root)
{
    if (root == nullptr) {
        return {};
    }
    cJSON* choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices)) {
        return {};
    }
    cJSON* choice = cJSON_GetArrayItem(choices, 0);
    if (!cJSON_IsObject(choice)) {
        return {};
    }
    cJSON* message = cJSON_GetObjectItemCaseSensitive(choice, "message");
    if (!cJSON_IsObject(message)) {
        return {};
    }
    return JsonStringField(message, "content");
}

void AppendLe16(uint16_t value, std::array<uint8_t, 44>* out, size_t* offset)
{
    if (out == nullptr || offset == nullptr || *offset + 2U > out->size()) {
        return;
    }
    (*out)[(*offset)++] = static_cast<uint8_t>(value & 0xFF);
    (*out)[(*offset)++] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void AppendLe32(uint32_t value, std::array<uint8_t, 44>* out, size_t* offset)
{
    if (out == nullptr || offset == nullptr || *offset + 4U > out->size()) {
        return;
    }
    (*out)[(*offset)++] = static_cast<uint8_t>(value & 0xFF);
    (*out)[(*offset)++] = static_cast<uint8_t>((value >> 8) & 0xFF);
    (*out)[(*offset)++] = static_cast<uint8_t>((value >> 16) & 0xFF);
    (*out)[(*offset)++] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

std::array<uint8_t, 44> BuildWavHeaderPcm16Mono(size_t sample_count, uint32_t sample_rate_hz)
{
    constexpr uint16_t kChannels = 1;
    constexpr uint16_t kBitsPerSample = 16;
    constexpr uint16_t kBlockAlign = kChannels * (kBitsPerSample / 8U);
    const uint32_t data_bytes = static_cast<uint32_t>(sample_count * sizeof(int16_t));
    const uint32_t byte_rate = sample_rate_hz * kBlockAlign;

    std::array<uint8_t, 44> header = {};
    size_t offset = 0;
    header[offset++] = 'R';
    header[offset++] = 'I';
    header[offset++] = 'F';
    header[offset++] = 'F';
    AppendLe32(36U + data_bytes, &header, &offset);
    header[offset++] = 'W';
    header[offset++] = 'A';
    header[offset++] = 'V';
    header[offset++] = 'E';
    header[offset++] = 'f';
    header[offset++] = 'm';
    header[offset++] = 't';
    header[offset++] = ' ';
    AppendLe32(16U, &header, &offset);
    AppendLe16(1U, &header, &offset);
    AppendLe16(kChannels, &header, &offset);
    AppendLe32(sample_rate_hz, &header, &offset);
    AppendLe32(byte_rate, &header, &offset);
    AppendLe16(kBlockAlign, &header, &offset);
    AppendLe16(kBitsPerSample, &header, &offset);
    header[offset++] = 'd';
    header[offset++] = 'a';
    header[offset++] = 't';
    header[offset++] = 'a';
    AppendLe32(data_bytes, &header, &offset);
    return header;
}

// Streams a single WAV body (header + PCM chunks) to the local transcription endpoint. Unlike
// the old Gemini path this is one plain POST -- no resumable-upload session negotiation, no
// separate generateContent call.
HttpResponse PostWavClip(const std::string& transcribe_url,
                         const recording_service::RecordedClip& clip)
{
    HttpResponse response = {};

    esp_http_client_config_t config = {};
    config.url = transcribe_url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = kTranscribeTimeoutMs;
    // No event_handler here, unlike PerformGet/PerformJsonPost: this function reads the
    // response body manually below (it also has to write the request body manually, in
    // chunks, since it streams a WAV clip rather than sending one string). Wiring an
    // HTTP_EVENT_ON_DATA handler on top of that manual read loop would double-append every
    // response byte -- the event fires during esp_http_client_read() too, not just _perform().

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        response.error_code = "http_client_init_failed";
        response.error_message = "Failed to initialize local AI transcription client";
        return response;
    }

    const size_t total_bytes = clip.wav_byte_count();
    char content_length[32] = {};
    std::snprintf(content_length, sizeof(content_length), "%u", static_cast<unsigned>(total_bytes));
    esp_http_client_set_header(client, "Content-Type", "audio/wav");
    esp_http_client_set_header(client, "Content-Length", content_length);
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "User-Agent", "folloup-sticky");

    esp_err_t err = esp_http_client_open(client, static_cast<int>(total_bytes));
    if (err != ESP_OK) {
        response.error_code = "transport_error";
        response.error_message = esp_err_to_name(err);
        esp_http_client_cleanup(client);
        return response;
    }

    const std::array<uint8_t, 44> header =
        BuildWavHeaderPcm16Mono(clip.sample_count(), clip.sample_rate_hz());
    const int header_written = esp_http_client_write(
        client, reinterpret_cast<const char*>(header.data()), static_cast<int>(header.size()));
    if (header_written != static_cast<int>(header.size())) {
        response.error_code = "transport_error";
        response.error_message = "Failed writing WAV header";
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return response;
    }

    bool write_failed = false;
    clip.ForEachChunk([&](const int16_t* chunk_data, size_t chunk_size) {
        if (write_failed || chunk_data == nullptr || chunk_size == 0) {
            return;
        }
        const int bytes_to_write = static_cast<int>(chunk_size * sizeof(int16_t));
        const int written =
            esp_http_client_write(client, reinterpret_cast<const char*>(chunk_data), bytes_to_write);
        if (written != bytes_to_write) {
            write_failed = true;
        }
    });
    if (write_failed) {
        response.error_code = "transport_error";
        response.error_message = "Failed streaming audio to the local transcription endpoint";
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return response;
    }

    const int response_length = esp_http_client_fetch_headers(client);
    response.status_code = esp_http_client_get_status_code(client);
    if (response.status_code <= 0 && response_length < 0) {
        response.error_code = "transport_error";
        response.error_message = "Failed fetching local transcription response headers";
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return response;
    }

    std::array<char, 512> buffer = {};
    while (true) {
        const int read = esp_http_client_read(client, buffer.data(), buffer.size());
        if (read < 0) {
            response.error_code = "transport_error";
            response.error_message = "Failed reading local transcription response body";
            break;
        }
        if (read == 0) {
            break;
        }
        response.body.append(buffer.data(), static_cast<size_t>(read));
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return response;
}

}  // namespace

esp_err_t Init()
{
    Snapshot snapshot = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_initialized) {
            return ESP_OK;
        }

        const StoredUrls stored = LoadStoredUrls();
        s_stored_base_url = stored.base_url;
        s_stored_transcribe_url = stored.transcribe_url;
        s_last_status_message =
            GetEffectiveBaseUrlLocked().empty()
                ? "No local AI server configured"
                : "Local AI server configured";
        ClearLastErrorLocked();
        s_initialized = true;
        snapshot = BuildSnapshotLocked();
    }

    ESP_LOGI(kTag, "Local AI service initialized: configured=%d source=%s base_url=%s",
             snapshot.settings.configured ? 1 : 0,
             UrlSourceName(snapshot.settings.base_url_source),
             snapshot.settings.base_url.empty() ? "<none>" : snapshot.settings.base_url.c_str());
    Notify();
    MaybeBeginAuthentication();
    return ESP_OK;
}

void SetEventHandler(EventHandler handler, void* context)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_event_handler = handler;
    s_event_context = context;
}

Snapshot GetSnapshot()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return BuildSnapshotLocked();
}

Result ApplySettingsPatch(const SettingsPatch& patch)
{
    bool should_start_auth = false;
    bool save_failed = false;
    StoredUrls next = {};
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_initialized) {
            const StoredUrls stored = LoadStoredUrls();
            s_stored_base_url = stored.base_url;
            s_stored_transcribe_url = stored.transcribe_url;
            s_initialized = true;
        }

        if (!patch.has_base_url && !patch.has_transcribe_url) {
            return {
                .success = false,
                .validation_error = true,
                .status_code = 400,
                .field = "base_url",
                .error_code = "missing_fields",
                .message = "At least one of base_url or transcribe_url is required",
            };
        }

        next.base_url = patch.has_base_url ? NormalizeBaseUrl(patch.base_url) : s_stored_base_url;
        next.transcribe_url =
            patch.has_transcribe_url ? TrimCopy(patch.transcribe_url) : s_stored_transcribe_url;

        if (patch.has_base_url && next.base_url.empty()) {
            return {
                .success = false,
                .validation_error = true,
                .status_code = 400,
                .field = "base_url",
                .error_code = "invalid_base_url",
                .message = "base_url must not be empty",
            };
        }

        if (patch.has_transcribe_url && next.transcribe_url.empty()) {
            return {
                .success = false,
                .validation_error = true,
                .status_code = 400,
                .field = "transcribe_url",
                .error_code = "invalid_transcribe_url",
                .message = "transcribe_url must not be empty",
            };
        }

        if (!SaveStoredUrls(next)) {
            SetLastErrorLocked("nvs_write_failed", "Failed to store local AI settings");
            save_failed = true;
        } else {
            s_stored_base_url = next.base_url;
            s_stored_transcribe_url = next.transcribe_url;
            ++s_auth_generation;
            s_request_in_flight = false;
            s_auth_checked = false;
            s_authenticated = false;
            s_last_http_status = 0;
            s_last_status_message = "Local AI settings stored";
            s_last_model_resource_name.clear();
            s_last_model_display_name.clear();
            ClearLastErrorLocked();
            should_start_auth = ShouldStartAuthenticationLocked();
        }
    }

    if (save_failed) {
        Notify();
        return {
            .success = false,
            .validation_error = false,
            .status_code = 500,
            .field = "base_url",
            .error_code = "nvs_write_failed",
            .message = "Failed to store local AI settings",
        };
    }

    Notify();
    if (should_start_auth) {
        (void)BeginAuthentication();
    }
    return {
        .success = true,
        .validation_error = false,
        .status_code = 200,
        .field = {},
        .error_code = {},
        .message = "Local AI settings stored",
    };
}

Result ResetStoredSettings()
{
    bool should_start_auth = false;
    bool clear_failed = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_initialized) {
            const StoredUrls stored = LoadStoredUrls();
            s_stored_base_url = stored.base_url;
            s_stored_transcribe_url = stored.transcribe_url;
            s_initialized = true;
        }

        if (!ClearStoredUrlsFromNvs()) {
            SetLastErrorLocked("nvs_clear_failed", "Failed to clear local AI settings");
            clear_failed = true;
        } else {
            s_stored_base_url.clear();
            s_stored_transcribe_url.clear();
            ++s_auth_generation;
            s_request_in_flight = false;
            s_auth_checked = false;
            s_authenticated = false;
            s_last_http_status = 0;
            s_last_status_message = "Local AI settings reset to built-in defaults";
            s_last_model_resource_name.clear();
            s_last_model_display_name.clear();
            ClearLastErrorLocked();
            should_start_auth = ShouldStartAuthenticationLocked();
        }
    }

    Notify();
    if (clear_failed) {
        return {
            .success = false,
            .validation_error = false,
            .status_code = 500,
            .field = "base_url",
            .error_code = "nvs_clear_failed",
            .message = "Failed to clear local AI settings",
        };
    }

    if (should_start_auth) {
        (void)BeginAuthentication();
    }
    return {
        .success = true,
        .validation_error = false,
        .status_code = 200,
        .field = {},
        .error_code = {},
        .message = "Local AI settings reset to built-in defaults",
    };
}

std::string GetEffectiveBaseUrl()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return GetEffectiveBaseUrlLocked();
}

std::string GetEffectiveTranscribeUrl()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return GetEffectiveTranscribeUrlLocked();
}

std::string GetEffectiveModelName()
{
    return kDefaultModelName;
}

TextResult GenerateText(const std::string& prompt)
{
    TextResult result = {};
    const std::string base_url = GetEffectiveBaseUrl();
    const std::string model_name = GetEffectiveModelName();
    if (base_url.empty() || model_name.empty()) {
        result.error_code = "not_configured";
        result.error_message = "No local AI server configured";
        return result;
    }
    if (prompt.empty()) {
        result.error_code = "empty_prompt";
        result.error_message = "Prompt was empty";
        return result;
    }

    const std::string url = base_url + "chat/completions";
    const HttpResponse http =
        PerformJsonPost(url, BuildChatCompletionRequestBody(model_name, prompt), kGenerateTimeoutMs);
    result.http_status = http.status_code;
    if (!http.error_code.empty()) {
        result.error_code = http.error_code;
        result.error_message = TrimForLog(http.error_message);
    } else {
        cJSON* root = cJSON_ParseWithLength(http.body.c_str(), http.body.size());
        if (http.status_code >= 200 && http.status_code < 300) {
            result.text = ExtractChatCompletionText(root);
            result.success = !result.text.empty();
            if (!result.success) {
                result.error_code = "empty_response";
                result.error_message = "Local AI server returned no text";
            }
        } else {
            PopulateHttpError(root, http, &result.error_code, &result.error_message);
        }
        if (root != nullptr) {
            cJSON_Delete(root);
        }
    }

    if (result.success) {
        ESP_LOGI(kTag, "Local AI chat/completions succeeded: http=%d chars=%u", result.http_status,
                 static_cast<unsigned>(result.text.size()));
    } else {
        ESP_LOGW(kTag, "Local AI chat/completions failed: http=%d code=%s message=%s",
                 result.http_status,
                 result.error_code.empty() ? "<none>" : result.error_code.c_str(),
                 result.error_message.empty() ? "<none>" : result.error_message.c_str());
    }
    return result;
}

TranscriptionResult Transcribe(const recording_service::RecordedClip& clip)
{
    TranscriptionResult result = {};
    result.clip_duration_ms = clip.duration_ms();
    result.wav_bytes = clip.wav_byte_count();

    const std::string transcribe_url = GetEffectiveTranscribeUrl();
    if (transcribe_url.empty()) {
        result.error_code = "not_configured";
        result.error_message = "No local transcription endpoint configured";
        return result;
    }
    if (clip.empty()) {
        result.error_code = "empty_audio";
        result.error_message = "No recorded audio available";
        return result;
    }

    const int64_t task_started_us = esp_timer_get_time();
    const HttpResponse http = PostWavClip(transcribe_url, clip);
    result.http_status = http.status_code;
    result.total_elapsed_ms =
        static_cast<uint64_t>((esp_timer_get_time() - task_started_us) / 1000ULL);

    if (!http.error_code.empty()) {
        result.error_code = http.error_code;
        result.error_message = TrimForLog(http.error_message);
        return result;
    }

    cJSON* root = cJSON_ParseWithLength(http.body.c_str(), http.body.size());
    if (http.status_code >= 200 && http.status_code < 300) {
        result.transcript = TrimForLog(JsonStringField(root, "transcript"), 1U << 20);
        result.success = !result.transcript.empty();
        if (!result.success) {
            result.error_code = "empty_transcript";
            result.error_message = "Local transcription endpoint returned no transcript text";
        }
    } else {
        PopulateHttpError(root, http, &result.error_code, &result.error_message);
    }
    if (root != nullptr) {
        cJSON_Delete(root);
    }
    return result;
}

bool BeginAuthentication()
{
    std::string base_url;
    std::string model_name;
    UrlSource url_source = UrlSource::kBuiltIn;
    uint32_t auth_generation = 0;
    bool missing_base_url = false;
    bool task_alloc_failed = false;
    bool task_start_failed = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_initialized) {
            const StoredUrls stored = LoadStoredUrls();
            s_stored_base_url = stored.base_url;
            s_stored_transcribe_url = stored.transcribe_url;
            s_initialized = true;
        }
        if (s_request_in_flight) {
            return false;
        }

        base_url = GetEffectiveBaseUrlLocked();
        if (base_url.empty()) {
            s_request_in_flight = false;
            s_auth_checked = false;
            s_authenticated = false;
            s_last_http_status = 0;
            s_last_status_message = "Readiness check skipped";
            s_last_model_resource_name.clear();
            s_last_model_display_name.clear();
            SetLastErrorLocked("not_configured", "No local AI server configured");
            missing_base_url = true;
        } else if (!s_network_connected) {
            return false;
        }

        if (!missing_base_url) {
            s_request_in_flight = true;
            s_auth_checked = false;
            s_authenticated = false;
            s_last_http_status = 0;
            s_last_status_message = "Checking local AI server";
            s_last_model_resource_name.clear();
            s_last_model_display_name.clear();
            ClearLastErrorLocked();
            model_name = GetEffectiveModelName();
            url_source = GetUrlSourceLocked();
            auth_generation = ++s_auth_generation;
        }
    }

    if (missing_base_url) {
        Notify();
        return false;
    }

    Notify();

    std::unique_ptr<AuthTaskContext> context(new (std::nothrow) AuthTaskContext{
        .base_url = std::move(base_url),
        .model_name = std::move(model_name),
        .generation = auth_generation,
    });
    if (context == nullptr) {
        task_alloc_failed = true;
    } else {
        TaskHandle_t task_handle = nullptr;
        const BaseType_t created = xTaskCreatePinnedToCore(
            AuthenticationTask,
            "local_ai_auth",
            kAuthTaskStackWords,
            context.get(),
            followup_task_config::kPriorityLocalAi,
            &task_handle,
            followup_task_config::kSystemCore);
        if (created != pdPASS || task_handle == nullptr) {
            task_start_failed = true;
        } else {
            context.release();
        }
    }

    if (task_alloc_failed || task_start_failed) {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_request_in_flight = false;
            s_last_status_message = "Failed to start local AI readiness check";
            SetLastErrorLocked(task_alloc_failed ? "task_alloc_failed" : "task_start_failed",
                               task_alloc_failed
                                   ? "Failed to allocate local AI task context"
                                   : "Failed to start local AI readiness task");
        }
        Notify();
        return false;
    }

    ESP_LOGI(kTag, "Starting local AI readiness check (model=%s, source=%s)",
             GetEffectiveModelName().c_str(),
             UrlSourceName(url_source));
    return true;
}

void SetNetworkState(bool connected, bool access_point_mode)
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_network_connected = connected;
        s_access_point_mode = access_point_mode;
    }
    MaybeBeginAuthentication();
}

void RegisterPortalRoutes(httpd_handle_t server)
{
    if (server == nullptr) {
        return;
    }

    httpd_uri_t settings_get = {
        .uri = kPortalApiSettingsUri,
        .method = HTTP_GET,
        .handler = HandlePortalSettingsGet,
        .user_ctx = nullptr,
    };
    httpd_uri_t settings_patch = {
        .uri = kPortalApiSettingsUri,
        .method = HTTP_PATCH,
        .handler = HandlePortalSettingsPatch,
        .user_ctx = nullptr,
    };
    httpd_uri_t settings_reset = {
        .uri = kPortalApiSettingsResetUri,
        .method = HTTP_POST,
        .handler = HandlePortalSettingsReset,
        .user_ctx = nullptr,
    };
    httpd_uri_t runtime_get = {
        .uri = kPortalApiRuntimeUri,
        .method = HTTP_GET,
        .handler = HandlePortalRuntimeGet,
        .user_ctx = nullptr,
    };

    if (RegisterPortalRoute(server, &settings_get) != ESP_OK ||
        RegisterPortalRoute(server, &settings_patch) != ESP_OK ||
        RegisterPortalRoute(server, &settings_reset) != ESP_OK ||
        RegisterPortalRoute(server, &runtime_get) != ESP_OK) {
        ESP_LOGW(kTag, "Local AI portal routes are incomplete");
    }
}

const char* UrlSourceName(UrlSource source)
{
    switch (source) {
        case UrlSource::kNvs:
            return "nvs";
        case UrlSource::kBuiltIn:
        default:
            return "built_in";
    }
}

}  // namespace local_ai_service
