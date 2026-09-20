#include "archive_portal.h"

#include <string>
#include <vector>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "joint_tracker_service.h"
#include "recording_archive_service.h"
#include "summary_service.h"

namespace archive_portal {
namespace {

constexpr const char* kTag = "ArchivePortal";
constexpr const char* kApiRecordingsUri = "/api/archive/recordings";
constexpr const char* kApiSummariesUri = "/api/archive/summaries";
// Einzige schreibende Route hier -- und sie schreibt nichts, sie stoesst an.
// Sie ist der Handgriff, der die Pruefzeile "eine volle Zusammenfassung geht
// durch" ueberhaupt erst ohne einen Menschen am Geraet ausfuehrbar macht:
// vorher musste jemand den Knopf druecken, und danach war der Lauf schon
// vorbei. Loeschen und Aendern bleiben ausdruecklich draussen.
constexpr const char* kApiSummarizeUri = "/api/archive/summarize";
// Nur-lesende Route fuer den 420-Track: heutiger Stand, Tagesziel und das
// Protokoll der vergangenen Tage. Der Pi-Export zieht das in eine Vault-Datei.
constexpr const char* kApiJointTrackUri = "/api/420track";

// Das Geraet haelt die Zusammenfassungen ohnehin im Schnappschuss; die
// Aufnahmen kommen frisch von der Karte. Beides landet unveraendert im
// JSON -- diese Routen deuten nichts, sie reichen durch.

std::string JsonString(cJSON* root)
{
    if (root == nullptr) {
        return "{}";
    }
    char* raw = cJSON_PrintUnformatted(root);
    if (raw == nullptr) {
        return "{}";
    }
    std::string out(raw);
    cJSON_free(raw);
    return out;
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
        default:
            httpd_resp_set_status(request, HTTPD_500);
            break;
    }
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    return httpd_resp_send(request, payload.c_str(), payload.size());
}

// `?transcripts=0` laesst die Transkripttexte weg. Das ist keine Kosmetik:
// ListRecordings liest dafuer pro Aufnahme eine eigene Datei von der Karte,
// und wer nur wissen will, wie viele Aufnahmen ueberhaupt ein Transkript
// haben, soll dafuer nicht den ganzen Text durchs Netz ziehen muessen.
bool WantsTranscripts(httpd_req_t* request)
{
    const size_t query_len = httpd_req_get_url_query_len(request);
    if (query_len == 0) {
        return true;
    }
    std::string query(query_len + 1, '\0');
    if (httpd_req_get_url_query_str(request, query.data(), query.size()) != ESP_OK) {
        return true;
    }
    char value[8] = {};
    if (httpd_query_key_value(query.c_str(), "transcripts", value, sizeof(value)) != ESP_OK) {
        return true;
    }
    return !(value[0] == '0' || value[0] == 'f' || value[0] == 'n');
}

void AppendMetadata(cJSON* target, const recording_archive_service::RecordingMetadata& metadata)
{
    cJSON_AddStringToObject(target, "tag", recording_archive_service::TagName(metadata.tag));
    cJSON_AddNumberToObject(target, "created_unix_seconds",
                            static_cast<double>(metadata.created_unix_seconds));
    cJSON_AddStringToObject(target, "created_local_date", metadata.created_local_date.c_str());
    cJSON_AddBoolToObject(target, "time_valid", metadata.time_valid);
    cJSON_AddNumberToObject(target, "duration_ms", static_cast<double>(metadata.duration_ms));
    cJSON_AddBoolToObject(target, "has_transcript", metadata.has_transcript);
    cJSON_AddBoolToObject(target, "completed", metadata.completed);
    cJSON_AddBoolToObject(target, "follow_up", metadata.follow_up);
    cJSON_AddBoolToObject(target, "follow_up_completed", metadata.follow_up_completed);
}

esp_err_t HandleRecordings(httpd_req_t* request)
{
    const bool with_transcripts = WantsTranscripts(request);

    esp_err_t status = ESP_OK;
    const std::vector<recording_archive_service::RecordingEntry> entries =
        recording_archive_service::ListRecordings(&status, with_transcripts);

    cJSON* root = cJSON_CreateObject();
    // Eine leere Liste von einem Lesefehler unterscheidbar halten -- sonst
    // sieht eine kaputte Karte aus wie ein frisch formatiertes Geraet.
    cJSON_AddBoolToObject(root, "ok", status == ESP_OK);
    cJSON_AddStringToObject(root, "status", esp_err_to_name(status));
    cJSON_AddBoolToObject(root, "transcripts_included", with_transcripts);
    cJSON_AddNumberToObject(root, "count", static_cast<double>(entries.size()));

    const recording_archive_service::Snapshot snapshot =
        recording_archive_service::GetSnapshot();
    cJSON* counts = cJSON_AddObjectToObject(root, "counts");
    cJSON_AddNumberToObject(counts, "recordings", snapshot.recording_count);
    cJSON_AddNumberToObject(counts, "notes", snapshot.notes_recording_count);
    cJSON_AddNumberToObject(counts, "todos", snapshot.todo_recording_count);
    cJSON_AddNumberToObject(counts, "follow_ups", snapshot.follow_up_recording_count);
    cJSON_AddNumberToObject(counts, "todos_completed", snapshot.completed_todo_count);
    cJSON_AddNumberToObject(counts, "todos_open", snapshot.incomplete_todo_count);

    cJSON* items = cJSON_AddArrayToObject(root, "recordings");
    for (const recording_archive_service::RecordingEntry& entry : entries) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "recording_id", entry.recording_id.c_str());
        cJSON_AddNumberToObject(item, "modified_unix_seconds",
                                static_cast<double>(entry.modified_unix_seconds));
        AppendMetadata(item, entry.metadata);
        if (with_transcripts) {
            cJSON_AddStringToObject(item, "transcript", entry.transcript_text.c_str());
        }
        cJSON_AddItemToArray(items, item);
    }

    ESP_LOGI(kTag, "Archive read: count=%u transcripts=%d status=%s",
             static_cast<unsigned>(entries.size()), with_transcripts ? 1 : 0,
             esp_err_to_name(status));
    return SendJsonResponse(request, 200, root);
}

void AppendCacheEntry(cJSON* target, const char* name,
                      const summary_service::CacheEntrySnapshot& entry)
{
    cJSON* node = cJSON_AddObjectToObject(target, name);
    cJSON_AddBoolToObject(node, "available", entry.available);
    cJSON_AddStringToObject(node, "text", entry.text.c_str());
    cJSON* meta = cJSON_AddObjectToObject(node, "metadata");
    cJSON_AddNumberToObject(meta, "generated_unix_seconds",
                            static_cast<double>(entry.metadata.generated_unix_seconds));
    cJSON_AddNumberToObject(meta, "source_item_count", entry.metadata.source_item_count);
    cJSON_AddNumberToObject(meta, "transcript_item_count", entry.metadata.transcript_item_count);
    cJSON_AddNumberToObject(meta, "missing_transcript_item_count",
                            entry.metadata.missing_transcript_item_count);
    cJSON_AddBoolToObject(meta, "truncated", entry.metadata.truncated);
    cJSON_AddBoolToObject(meta, "chunked", entry.metadata.chunked);
    cJSON_AddNumberToObject(meta, "window_days", entry.metadata.window_days);
}

esp_err_t HandleSummaries(httpd_req_t* request)
{
    // Erst von der Karte nachziehen: der Schnappschuss im Speicher kann
    // aelter sein als die Datei, wenn seit dem Start eine Zusammenfassung
    // dazugekommen ist.
    (void)summary_service::RefreshCachedSummaries();
    const summary_service::Snapshot snapshot = summary_service::GetSnapshot();

    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "initialized", snapshot.initialized);
    cJSON_AddBoolToObject(root, "storage_available", snapshot.storage_available);
    AppendCacheEntry(root, "notes", snapshot.notes);
    AppendCacheEntry(root, "todos", snapshot.todos);

    // Ein laufender oder gescheiterter Lauf gehoert mit ins Bild -- sonst
    // sieht eine abgebrochene Zusammenfassung aus wie gar keine.
    cJSON* req = cJSON_AddObjectToObject(root, "request");
    cJSON_AddBoolToObject(req, "in_flight", snapshot.request.in_flight);
    cJSON_AddStringToObject(req, "kind",
                            summary_service::SummaryKindName(snapshot.request.kind));
    cJSON_AddStringToObject(req, "status_message", snapshot.request.status_message.c_str());
    cJSON_AddStringToObject(req, "error_code", snapshot.request.error_code.c_str());
    cJSON_AddStringToObject(req, "error_message", snapshot.request.error_message.c_str());

    ESP_LOGI(kTag, "Summary read: notes=%d todos=%d in_flight=%d",
             snapshot.notes.available ? 1 : 0, snapshot.todos.available ? 1 : 0,
             snapshot.request.in_flight ? 1 : 0);
    return SendJsonResponse(request, 200, root);
}

// POST /api/archive/summarize?kind=todos|notes
// Stoesst denselben Lauf an, den der Knopf am Geraet ausloest -- kein eigener
// Pfad daneben, sondern derselbe RequestSummary()-Aufruf. Antwortet sofort;
// das Ergebnis kommt ueber /api/archive/summaries, wenn es fertig ist.
esp_err_t HandleSummarize(httpd_req_t* request)
{
    summary_service::SummaryKind kind = summary_service::SummaryKind::kTodos;
    const size_t query_len = httpd_req_get_url_query_len(request);
    if (query_len > 0) {
        std::string query(query_len + 1, '\0');
        char value[16] = {};
        if (httpd_req_get_url_query_str(request, query.data(), query.size()) == ESP_OK &&
            httpd_query_key_value(query.c_str(), "kind", value, sizeof(value)) == ESP_OK &&
            std::string(value) == "notes") {
            kind = summary_service::SummaryKind::kNotes;
        }
    }

    const bool angenommen = summary_service::RequestSummary(kind);
    cJSON* root = cJSON_CreateObject();
    // Abgelehnt heisst fast immer: es laeuft schon einer. Das ist kein Fehler,
    // sondern die Warteschlange, die ihre Arbeit tut -- deshalb 200 und ein
    // ehrliches Feld statt eines Fehlercodes.
    cJSON_AddBoolToObject(root, "angenommen", angenommen);
    cJSON_AddStringToObject(root, "kind", summary_service::SummaryKindName(kind));
    const summary_service::Snapshot snapshot = summary_service::GetSnapshot();
    cJSON_AddBoolToObject(root, "laeuft_bereits", snapshot.request.in_flight);

    ESP_LOGI(kTag, "Summary requested via portal: kind=%s accepted=%d",
             summary_service::SummaryKindName(kind), angenommen ? 1 : 0);
    return SendJsonResponse(request, 200, root);
}

// GET /api/420track
// Nur lesend: heutiger Stand, Tagesziel und das ganze Protokoll (aeltester Tag
// zuerst, der heutige Tag als letzter Eintrag). Der Pi-Export baut daraus eine
// Vault-Datei; hier wird nichts gedeutet, nur durchgereicht.
esp_err_t HandleJointTrack(httpd_req_t* request)
{
    const int goal = joint_tracker_service::GetDailyGoal();
    const int today_count = joint_tracker_service::GetTodayCount();
    const std::vector<joint_tracker_service::DayCount> history =
        joint_tracker_service::GetHistory();

    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", true);
    cJSON_AddNumberToObject(root, "goal", goal);
    cJSON_AddNumberToObject(root, "today_count", today_count);
    // Der heutige Tag steht als letzter Eintrag in history; als eigenes Feld,
    // damit der Export ihn ohne Sonderfall greifen kann. 0 = Uhr noch ungestellt.
    const int32_t today_day = history.empty() ? 0 : history.back().day;
    cJSON_AddNumberToObject(root, "today_day", static_cast<double>(today_day));

    cJSON* days = cJSON_AddArrayToObject(root, "days");
    for (const joint_tracker_service::DayCount& entry : history) {
        cJSON* node = cJSON_CreateObject();
        cJSON_AddNumberToObject(node, "day", static_cast<double>(entry.day));
        cJSON_AddNumberToObject(node, "count", entry.count);
        cJSON_AddItemToArray(days, node);
    }

    ESP_LOGI(kTag, "420track read: today=%d goal=%d days=%u", today_count, goal,
             static_cast<unsigned>(history.size()));
    return SendJsonResponse(request, 200, root);
}

void RegisterRoute(httpd_handle_t server, const httpd_uri_t* handler)
{
    const esp_err_t err = httpd_register_uri_handler(server, handler);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "Failed to register archive route %s: %s", handler->uri,
                 esp_err_to_name(err));
    }
}

}  // namespace

void RegisterPortalRoutes(httpd_handle_t server)
{
    if (server == nullptr) {
        return;
    }

    httpd_uri_t recordings = {
        .uri = kApiRecordingsUri,
        .method = HTTP_GET,
        .handler = HandleRecordings,
        .user_ctx = nullptr,
    };
    httpd_uri_t summaries = {
        .uri = kApiSummariesUri,
        .method = HTTP_GET,
        .handler = HandleSummaries,
        .user_ctx = nullptr,
    };

    httpd_uri_t summarize = {
        .uri = kApiSummarizeUri,
        .method = HTTP_POST,
        .handler = HandleSummarize,
        .user_ctx = nullptr,
    };

    httpd_uri_t joint_track = {
        .uri = kApiJointTrackUri,
        .method = HTTP_GET,
        .handler = HandleJointTrack,
        .user_ctx = nullptr,
    };

    RegisterRoute(server, &recordings);
    RegisterRoute(server, &summaries);
    RegisterRoute(server, &summarize);
    RegisterRoute(server, &joint_track);
}

}  // namespace archive_portal
