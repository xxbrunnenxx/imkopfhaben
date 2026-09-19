#include "device_status_service.h"

#include <mutex>

#include "cJSON.h"
#include "driver/temperature_sensor.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "local_ai_service.h"
#include "wifi_service.h"

namespace device_status_service {
namespace {

constexpr const char* kTag = "DeviceStatus";
constexpr int kHttpTimeoutMs = 4000;

std::mutex s_mutex;
Snapshot s_snapshot = {};
temperature_sensor_handle_t s_temp_sensor = nullptr;
bool s_temp_ready = false;

// Sammelt den HTTP-Body ueber die Event-Callbacks ein (wie local_ai_service).
struct HttpBody {
    std::string data;
};

esp_err_t HttpEventHandler(esp_http_client_event_t* event)
{
    if (event == nullptr) {
        return ESP_FAIL;
    }
    auto* body = static_cast<HttpBody*>(event->user_data);
    if (event->event_id == HTTP_EVENT_ON_DATA && body != nullptr && event->data != nullptr &&
        event->data_len > 0) {
        body->data.append(static_cast<const char*>(event->data),
                          static_cast<size_t>(event->data_len));
    }
    return ESP_OK;
}

// Leitet aus der Transkriptions-URL (".../api/transcribe-raw") die Status-URL
// (".../api/status") und den Anzeige-Host ab. Beides leer, wenn die URL nicht
// die erwartete Form hat.
void DeriveBrainUrls(const std::string& transcribe_url, std::string* status_url,
                     std::string* host_label)
{
    status_url->clear();
    host_label->clear();
    if (transcribe_url.empty()) {
        return;
    }

    // Schema abtrennen (http:// oder https://), um den Host herauszuloesen.
    const std::string::size_type scheme_end = transcribe_url.find("://");
    if (scheme_end == std::string::npos) {
        return;
    }
    const std::string::size_type host_start = scheme_end + 3;
    const std::string::size_type path_start = transcribe_url.find('/', host_start);
    const std::string host = (path_start == std::string::npos)
                                 ? transcribe_url.substr(host_start)
                                 : transcribe_url.substr(host_start, path_start - host_start);
    *host_label = host;

    // Status-URL an derselben Basis: Schema + Host + "/api/status".
    *status_url = transcribe_url.substr(0, host_start) + host + "/api/status";
}

bool ReadJsonNumber(cJSON* root, const char* key, double* out)
{
    cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == nullptr || !cJSON_IsNumber(item)) {
        return false;
    }
    *out = item->valuedouble;
    return true;
}

// Fuellt die Brain-Felder aus dem JSON-Body. Fehlende Felder bleiben ungueltig.
void ParseBrainStatus(const std::string& body, Snapshot* snapshot)
{
    cJSON* root = cJSON_Parse(body.c_str());
    if (root == nullptr) {
        ESP_LOGW(kTag, "Brain-Status: JSON nicht lesbar");
        return;
    }

    double value = 0.0;
    if (ReadJsonNumber(root, "brain_uptime_sekunden", &value)) {
        snapshot->brain_uptime_valid = true;
        snapshot->brain_uptime_seconds = value;
    }
    if (ReadJsonNumber(root, "pi_uptime_sekunden", &value)) {
        snapshot->pi_uptime_valid = true;
        snapshot->pi_uptime_seconds = value;
    }
    if (ReadJsonNumber(root, "pi_temperatur_celsius", &value)) {
        snapshot->pi_temp_valid = true;
        snapshot->pi_temp_celsius = static_cast<float>(value);
    }
    if (ReadJsonNumber(root, "cpu_last_prozent", &value)) {
        snapshot->cpu_load_valid = true;
        snapshot->cpu_load_percent = static_cast<float>(value);
    }
    if (ReadJsonNumber(root, "ram_prozent", &value)) {
        snapshot->ram_valid = true;
        snapshot->ram_percent = static_cast<float>(value);
    }

    cJSON_Delete(root);
}

// Holt /api/status. Setzt bei Erfolg die Brain-Felder und brain_reachable.
void FetchBrainStatus(const std::string& status_url, Snapshot* snapshot)
{
    HttpBody body = {};
    esp_http_client_config_t config = {};
    config.url = status_url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = kHttpTimeoutMs;
    config.event_handler = &HttpEventHandler;
    config.user_data = &body;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGW(kTag, "HTTP-Client konnte nicht erstellt werden");
        return;
    }
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "User-Agent", "folloup-sticky");

    const esp_err_t err = esp_http_client_perform(client);
    snapshot->brain_http_status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGI(kTag, "Brain nicht erreichbar: %s", esp_err_to_name(err));
        return;
    }
    if (snapshot->brain_http_status != 200) {
        ESP_LOGI(kTag, "Brain-Status HTTP %d", snapshot->brain_http_status);
        return;
    }

    snapshot->brain_reachable = true;
    ParseBrainStatus(body.data, snapshot);
}

}  // namespace

esp_err_t Init()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_temp_ready) {
        return ESP_OK;
    }
    // Bereich grosszuegig: der interne Sensor des ESP32-S3 misst die Chip-,
    // nicht die Umgebungstemperatur, die im Betrieb deutlich ueber Raum liegt.
    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 100);
    esp_err_t err = temperature_sensor_install(&config, &s_temp_sensor);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "Temperatursensor install fehlgeschlagen: %s", esp_err_to_name(err));
        s_temp_sensor = nullptr;
        return err;
    }
    err = temperature_sensor_enable(s_temp_sensor);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "Temperatursensor enable fehlgeschlagen: %s", esp_err_to_name(err));
        temperature_sensor_uninstall(s_temp_sensor);
        s_temp_sensor = nullptr;
        return err;
    }
    s_temp_ready = true;
    return ESP_OK;
}

void Refresh()
{
    Snapshot snapshot = {};

    // 1) Geraeteseite: WLAN-Status und eigene IP.
    const wifi_service::UiState wifi_state = wifi_service::GetUiState();
    snapshot.wifi_connected = wifi_state.connected;
    snapshot.device_ip = wifi_state.ip_address;
    snapshot.ssid = wifi_state.ssid;
    snapshot.rssi = wifi_state.rssi;

    // 2) Interne Chip-Temperatur.
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_temp_ready && s_temp_sensor != nullptr) {
            float celsius = 0.0f;
            if (temperature_sensor_get_celsius(s_temp_sensor, &celsius) == ESP_OK) {
                snapshot.esp_temp_valid = true;
                snapshot.esp_temp_celsius = celsius;
            }
        }
    }

    // 3) Brain-Seite: URL ableiten und /api/status holen -- nur mit Netz.
    std::string status_url;
    DeriveBrainUrls(local_ai_service::GetEffectiveTranscribeUrl(), &status_url,
                    &snapshot.brain_host);
    if (snapshot.wifi_connected && !status_url.empty()) {
        FetchBrainStatus(status_url, &snapshot);
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot = snapshot;
    }
}

Snapshot GetSnapshot()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_snapshot;
}

}  // namespace device_status_service
