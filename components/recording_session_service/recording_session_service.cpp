#include "recording_session_service.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "esp_log.h"
#include "esp_timer.h"
#include "followup_task_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "local_ai_service.h"
#include "playback_service.h"
#include "storage_service.h"
#include "system_sound_service.h"

namespace recording_session_service {
namespace {

constexpr const char* kTag = "RecordingSession";
constexpr const char* kIdleStatus = "Hold POWER to record";
constexpr const char* kArmedStatus = "Keep holding to record";
constexpr const char* kRecordingStatus = "Recording";
constexpr const char* kStartCueStatus = "Starting recording";
constexpr const char* kStopCueStatus = "Finishing recording";
constexpr const char* kPlayingBackStatus = "Playing back";
constexpr const char* kChooseTagStatus = "Choose recording type";
constexpr const char* kSavingStatus = "Saving recording";
constexpr const char* kTranscribingStatus = "Transcribing recording";
constexpr const char* kSavedWithoutTranscriptStatus =
    "Recording saved without transcription";
constexpr const char* kDiscardedStatus = "Recording discarded";
constexpr uint32_t kMinTranscriptionDurationMs = 500;
constexpr uint32_t kFallbackAudioSampleRateHz = 24000;
// Prototype-level offline queue: no battery-aware backoff, just a flat interval. Covers the
// "device was carried around, deliver once back near Kraken" case from a plain periodic sweep,
// plus an immediate follow-on sweep after every successful save (see HandleTranscriptionEvent)
// so the backlog drains quickly once connectivity is confirmed, instead of waiting out the
// full interval.
constexpr uint64_t kTranscriptionRetryIntervalUs = 10ULL * 60ULL * 1000000ULL;
constexpr size_t kSignalWindowSamples = 240;
constexpr int32_t kSpeechPeakThreshold = 700;
constexpr size_t kMinSpeechWindows = 3;

constexpr std::array<TagOption, 4> kTagOptions = {{
    {.tag = recording_archive_service::RecordingTag::kNote, .label_text = "Note"},
    {.tag = recording_archive_service::RecordingTag::kTask, .label_text = "Task"},
    {.tag = recording_archive_service::RecordingTag::kIdea, .label_text = "Idea"},
    {.label_text = "Discard", .is_discard = true},
}};

struct GuardrailResult {
    bool accepted = false;
    const char* error_code = "";
    const char* status_message = "";
};

std::mutex s_mutex;
EventHandler s_event_handler = nullptr;
void* s_event_context = nullptr;
bool s_initialized = false;
Snapshot s_snapshot = {};
std::string s_pending_recording_id = {};

// Cue tokens make a late callback harmless: every cue we queue bumps the token, so a
// result that arrives after the session has moved on (cancel, failure, a new recording)
// no longer matches and is dropped instead of driving a stale transition.
uint32_t s_cue_token = 0;
std::atomic<bool> s_playback_worker_active{false};
// Set when BOOT is released before the start cue finishes; consumed by HandleStartCueResult.
bool s_finish_pending_after_start_cue = false;
esp_timer_handle_t s_retry_timer = nullptr;

// Finds the first archived recording still missing a transcript and re-runs it through the
// same load-clip -> transcribe -> save-transcript pipeline as the manual "retry" buttons
// (BeginArchivedTranscription, already used by details_page_runtime/vibe_check_page_runtime).
// One attempt per call, not a full sweep: BeginTranscription only ever runs one request at a
// time anyway, and this keeps a down/unreachable server from being hammered in a tight loop --
// the periodic timer and the post-success hook below call in again for the next entry.
void TryRetryOldestUnsentRecording()
{
    // Single snapshot instead of two separate readiness checks: transcription_service already
    // exports the "is a transcribe URL configured" result via provider_ready (see its own
    // TranscriptionEndpointConfigured()), no need for a third copy of that check here.
    const transcription_service::Snapshot ts = transcription_service::GetSnapshot();
    if (!ts.provider_ready) {
        return;  // nothing configured to retry against
    }
    if (ts.request_in_flight) {
        return;  // a recording or another retry is already running
    }
    for (const auto& entry : recording_archive_service::ListRecordings()) {
        if (!entry.metadata.has_transcript) {
            ESP_LOGI(kTag, "Retrying queued transcription: id=%s", entry.recording_id.c_str());
            BeginArchivedTranscription(entry.recording_id);
            return;
        }
    }
}

constexpr uint32_t kRetryKickTaskStackWords = 4096;

void RetryKickTask(void* /*arg*/)
{
    TryRetryOldestUnsentRecording();
    vTaskDelete(nullptr);
}

// Runs a retry pass on its own short-lived task instead of the caller's stack. Both callers
// below can run inside transcription_service's own NotifyLocked() (WorkerTask holds its
// s_mutex while calling the event handler chain that ends up here) -- calling
// TryRetryOldestUnsentRecording() directly would re-enter transcription_service::GetSnapshot(),
// which locks that same non-recursive mutex, and deadlock on the very first successful
// transcription. Spawning a task sidesteps the caller's lock entirely, the same pattern already
// used for playback/re-transcription workers elsewhere in this file.
void RequestRetryPass()
{
    xTaskCreatePinnedToCore(&RetryKickTask, "retry_kick", kRetryKickTaskStackWords, nullptr,
                            followup_task_config::kPriorityLocalAi, nullptr,
                            followup_task_config::kSystemCore);
}

void OnRetryTimer(void* /*arg*/)
{
    RequestRetryPass();
}

uint32_t ResolveClipDurationMs(const recording_service::RecordedClip& clip, uint32_t duration_ms)
{
    if (duration_ms > 0) {
        return duration_ms;
    }
    return clip.sample_rate_hz() > 0
               ? clip.duration_ms()
               : static_cast<uint32_t>((clip.sample_count() * 1000ULL) /
                                       kFallbackAudioSampleRateHz);
}

GuardrailResult ValidateClip(const recording_service::RecordedClip& clip, uint32_t duration_ms)
{
    if (clip.empty()) {
        return {
            .accepted = false,
            .error_code = "empty_audio",
            .status_message = "Recording too short",
        };
    }

    const uint32_t resolved_duration_ms = ResolveClipDurationMs(clip, duration_ms);
    if (resolved_duration_ms < kMinTranscriptionDurationMs) {
        return {
            .accepted = false,
            .error_code = "recording_too_short",
            .status_message = "Recording too short",
        };
    }

    size_t speech_windows = 0;
    size_t window_fill = 0;
    int32_t window_peak = 0;
    auto finish_window = [&]() -> bool {
        if (window_fill == 0) {
            return false;
        }
        if (window_peak >= kSpeechPeakThreshold) {
            ++speech_windows;
            if (speech_windows >= kMinSpeechWindows) {
                return true;
            }
        }
        window_fill = 0;
        window_peak = 0;
        return false;
    };

    bool accepted = false;
    clip.ForEachChunk([&](const int16_t* chunk_data, size_t chunk_size) {
        if (accepted || chunk_data == nullptr) {
            return;
        }
        for (size_t index = 0; index < chunk_size; ++index) {
            int32_t amplitude = chunk_data[index];
            if (amplitude < 0) {
                amplitude = -amplitude;
            }
            window_peak = std::max(window_peak, amplitude);
            ++window_fill;
            if (window_fill >= kSignalWindowSamples && finish_window()) {
                accepted = true;
                break;
            }
        }
    });
    if (!accepted && finish_window()) {
        accepted = true;
    }

    if (accepted) {
        return {
            .accepted = true,
            .error_code = "",
            .status_message = "",
        };
    }

    return {
        .accepted = false,
        .error_code = "recording_too_quiet",
        .status_message = "No speech detected",
    };
}

BlockedReason EvaluateBlockedReason(const Context& context)
{
    if (context.lock_screen_active) {
        return BlockedReason::kLockScreenActive;
    }
    if (context.overlay_visible) {
        return BlockedReason::kOverlayVisible;
    }
    if (storage_service::IsWriteBusy()) {
        return BlockedReason::kStorageBusy;
    }
    if (!recording_service::IsInitialized()) {
        return BlockedReason::kRecorderUnavailable;
    }
    if (transcription_service::GetSnapshot().request_in_flight) {
        return BlockedReason::kTranscriptionInFlight;
    }
    return BlockedReason::kNone;
}

const char* BlockedReasonStatusMessage(BlockedReason reason)
{
    switch (reason) {
        case BlockedReason::kLockScreenActive:
            return "Unlock to record";
        case BlockedReason::kOverlayVisible:
            return "Close the current dialog";
        case BlockedReason::kStorageBusy:
            return "Wait for SD activity to finish";
        case BlockedReason::kRecorderUnavailable:
            return "Recorder unavailable";
        case BlockedReason::kTranscriptionInFlight:
            return "Wait for transcription to finish";
        case BlockedReason::kNone:
        default:
            return "";
    }
}

void ResetToIdleLocked()
{
    s_snapshot.allowed = true;
    s_snapshot.blocked_reason = BlockedReason::kNone;
    s_snapshot.phase = Phase::kIdle;
    s_snapshot.has_clip = false;
    s_snapshot.clip_saved = false;
    s_snapshot.transcript_saved = false;
    s_snapshot.request_in_flight = false;
    s_snapshot.recorded_samples = 0;
    s_snapshot.duration_ms = 0;
    s_snapshot.input_level_percent = 0;
    s_snapshot.stop_reason = recording_service::StopReason::kNone;
    s_snapshot.last_saved_recording_id.clear();
    s_snapshot.last_saved_recording_path.clear();
    s_snapshot.last_saved_transcript_path.clear();
    s_snapshot.last_transcript.clear();
    s_snapshot.last_error_code.clear();
    s_snapshot.last_error_message.clear();
    s_snapshot.last_status_message = kIdleStatus;
    s_pending_recording_id.clear();
    // Invalidate any cue callback still in flight, so a late result cannot resurrect a
    // session that was just cancelled.
    ++s_cue_token;
    s_finish_pending_after_start_cue = false;
}

void SyncRecordingStateLocked(const recording_service::UiState& state)
{
    s_snapshot.initialized = state.initialized;
    s_snapshot.recorded_samples = state.recorded_samples;
    s_snapshot.duration_ms = state.duration_ms;
    s_snapshot.max_recording_ms = state.max_recording_ms;
    s_snapshot.input_level_percent = state.input_level_percent;
    s_snapshot.stop_reason = state.stop_reason;
    s_snapshot.has_clip = state.has_clip;
}

void NotifyLocked()
{
    EventHandler handler = s_event_handler;
    void* context = s_event_context;
    if (handler == nullptr) {
        return;
    }

    const Event event = {
        .snapshot = s_snapshot,
    };
    handler(event, context);
}

// Playback blocks for the length of the clip, so it cannot run on the sound-cue callback
// task. This mirrors the details page's short-lived worker: the shared_ptr keeps the PSRAM
// clip alive for the duration even though nothing else references it yet.
constexpr uint32_t kPlaybackWorkerStackWords = 4096;

void AdvanceToTagSelection(const char* reason);

void PlaybackWorker(void* arg)
{
    std::unique_ptr<recording_service::RecordedClipPtr> clip(
        static_cast<recording_service::RecordedClipPtr*>(arg));
    if (clip != nullptr && *clip != nullptr) {
        const esp_err_t err = playback_service::PlayClip(*clip);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "Clip playback failed: %s", esp_err_to_name(err));
        }
    }
    s_playback_worker_active.store(false, std::memory_order_release);
    AdvanceToTagSelection("playback finished");
    vTaskDelete(nullptr);
}

// Every route out of the stop cue lands here: playback done, playback refused to start, or
// the stop cue itself failed. The clip is still unsaved, so the tag menu's Discard option
// is what throws away a bad take.
void AdvanceToTagSelection(const char* reason)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_snapshot.phase != Phase::kStopCue && s_snapshot.phase != Phase::kPlayingBack) {
        return;
    }
    ESP_LOGI(kTag, "Advancing to tag selection (%s)", reason);
    s_snapshot.phase = Phase::kAwaitingTagSelection;
    s_snapshot.last_status_message = kChooseTagStatus;
    s_snapshot.last_error_code.clear();
    s_snapshot.last_error_message.clear();
    NotifyLocked();
}

// Replays the take the user just recorded. Returns false when playback could not be
// started, so the caller can fall through to tag selection rather than stranding the
// session in kStopCue.
bool StartClipPlayback(const recording_service::RecordedClipPtr& clip)
{
    if (clip == nullptr || clip->empty()) {
        return false;
    }
    bool expected = false;
    if (!s_playback_worker_active.compare_exchange_strong(expected, true)) {
        return false;
    }

    // Publish kPlayingBack before the worker can exist. The worker advances to tag
    // selection the moment it finishes, and a very short clip would otherwise beat this
    // update -- leaving the phase stuck on kPlayingBack after the menu had already opened.
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kPlayingBack;
        s_snapshot.last_status_message = kPlayingBackStatus;
        NotifyLocked();
    }

    auto* clip_copy = new recording_service::RecordedClipPtr(clip);
    if (xTaskCreate(&PlaybackWorker, "clip_playback", kPlaybackWorkerStackWords, clip_copy,
                    followup_task_config::kPriorityStorage, nullptr) != pdPASS) {
        delete clip_copy;
        s_playback_worker_active.store(false, std::memory_order_release);
        ESP_LOGW(kTag, "Failed to start clip playback worker");
        // Phase is kPlayingBack here, which AdvanceToTagSelection accepts, so the caller's
        // fallback still lands correctly.
        return false;
    }
    return true;
}

// Fires once the stop cue has finished (or failed). Playing the clip under the cue would
// overlap two streams on the same codec output, so the replay waits for it.
void HandleStopCueResult(uint32_t token, SoundCuePlaybackResult result)
{
    recording_service::RecordedClipPtr clip = nullptr;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (token != s_cue_token || s_snapshot.phase != Phase::kStopCue) {
            return;
        }
    }
    if (result != SoundCuePlaybackResult::kCompleted) {
        AdvanceToTagSelection("stop cue did not complete");
        return;
    }

    clip = recording_service::GetRecordedClip();
    if (!StartClipPlayback(clip)) {
        AdvanceToTagSelection("playback unavailable");
    }
}

void HandleStartCueResult(uint32_t token, SoundCuePlaybackResult)
{
    bool finish_now = false;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        // The cue is cosmetic: capture is already running underneath it, so even a failed
        // cue just moves the UI on rather than aborting the take.
        if (token != s_cue_token || s_snapshot.phase != Phase::kStartCue) {
            return;
        }
        finish_now = s_finish_pending_after_start_cue;
        s_finish_pending_after_start_cue = false;
        if (finish_now) {
            s_snapshot.phase = Phase::kSaving;
            s_snapshot.last_status_message = "Preparing recording";
        } else {
            s_snapshot.phase = Phase::kRecording;
            s_snapshot.last_status_message = kRecordingStatus;
        }
        NotifyLocked();
    }

    if (finish_now) {
        (void)recording_service::Finish();
    }
}

void MarkBlockedLocked(BlockedReason reason)
{
    s_snapshot.allowed = false;
    s_snapshot.blocked_reason = reason;
    s_snapshot.last_status_message = BlockedReasonStatusMessage(reason);
    s_snapshot.last_error_code.clear();
    s_snapshot.last_error_message.clear();
}

}  // namespace

esp_err_t Init()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_initialized) {
        return ESP_OK;
    }

    s_initialized = true;
    s_snapshot.initialized = recording_service::IsInitialized();
    s_snapshot.max_recording_ms = recording_service::GetUiState().max_recording_ms;
    ResetToIdleLocked();

    if (s_retry_timer == nullptr) {
        esp_timer_create_args_t timer_args = {};
        timer_args.callback = &OnRetryTimer;
        timer_args.dispatch_method = ESP_TIMER_TASK;
        timer_args.name = "transcribe_retry";
        timer_args.skip_unhandled_events = true;

        const esp_err_t timer_err = esp_timer_create(&timer_args, &s_retry_timer);
        if (timer_err == ESP_OK) {
            const esp_err_t start_err =
                esp_timer_start_periodic(s_retry_timer, kTranscriptionRetryIntervalUs);
            if (start_err != ESP_OK) {
                ESP_LOGW(kTag, "Retry timer start failed: %s", esp_err_to_name(start_err));
            }
        } else {
            ESP_LOGW(kTag, "Retry timer create failed: %s", esp_err_to_name(timer_err));
        }
    }

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
    return s_snapshot;
}

const std::array<TagOption, 4>& TagOptions()
{
    return kTagOptions;
}

bool BeginArchivedTranscription(const std::string& recording_id)
{
    if (Init() != ESP_OK || recording_id.empty()) {
        return false;
    }

    // Reuse the recording->transcription pipeline for an already-archived clip: load its WAV
    // back into memory, hand it to the same transcription service, and let the existing
    // HandleTranscriptionEvent path save the transcript and drive the toasts.
    if (transcription_service::GetSnapshot().request_in_flight) {
        return false;  // a recording or another re-transcribe is already running
    }

    {
        // Also called from the background retry (timer / post-save follow-up), which has no
        // idea what the user is doing right now. Only proceed when the session is genuinely at
        // rest -- clobbering phase/s_pending_recording_id mid-recording, mid-tag-selection, or
        // mid-save would silently drop whatever the user is actively doing (HandlePowerPressUp
        // and SubmitTagSelection both gate on the exact phase this would overwrite). The manual
        // retry buttons (details/vibe-check pages) only ever call this while the session is
        // already idle/complete/failed, so this doesn't change their behavior.
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_snapshot.phase != Phase::kIdle && s_snapshot.phase != Phase::kComplete &&
            s_snapshot.phase != Phase::kFailed) {
            return false;
        }
    }

    recording_service::RecordedClipPtr clip = recording_archive_service::LoadClip(recording_id);
    if (!clip || clip->empty()) {
        std::lock_guard<std::mutex> lock(s_mutex);
        // Only clobber if nothing else is genuinely transcribing right now: the request_in_flight
        // pre-check above and this call are two separate lock acquisitions (can't hold both this
        // mutex and transcription_service's at once without risking a lock-order deadlock with
        // WorkerTask, see RequestRetryPass), so a second, concurrent BeginArchivedTranscription
        // call can slip in between them. Without this guard a slow loser here could stomp a
        // faster winner's just-started kTranscribing state.
        if (s_snapshot.phase != Phase::kTranscribing) {
            s_snapshot.phase = Phase::kFailed;
            s_snapshot.request_in_flight = false;
            s_snapshot.last_status_message = "Couldn't load recording audio";
            s_snapshot.last_error_code = "clip_load_failed";
            s_snapshot.last_error_message = "Failed to read WAV from SD";
        }
        NotifyLocked();
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.clip_saved = true;  // the recording already lives on SD
        s_snapshot.transcript_saved = false;
    }

    if (transcription_service::BeginTranscription(clip)) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kTranscribing;
        s_snapshot.request_in_flight = true;
        // Set only on the winning path (transcription_service::BeginTranscription's own
        // atomic check-and-set is what decides winner/loser between two racing callers) -- a
        // losing concurrent call below would otherwise have already written its own id here
        // first and this write could get clobbered by its failure branch afterward, or vice
        // versa, depending on which call happens to finish first.
        s_pending_recording_id = recording_id;
        s_snapshot.last_status_message = kTranscribingStatus;
        s_snapshot.last_error_code.clear();
        s_snapshot.last_error_message.clear();
        NotifyLocked();
        return true;
    }

    // Couldn't start (e.g. no local transcription endpoint configured, or a concurrent call won
    // the race for transcription_service's single in-flight slot). Only clear the session's
    // transcribing state if it's still ours -- i.e. nothing else has since claimed
    // s_pending_recording_id -- so a loser here can never erase a winner's in-progress
    // transcription and strand its eventual transcript unattached (see HandleTranscriptionEvent,
    // which requires phase == kTranscribing and a matching s_pending_recording_id to save it).
    const transcription_service::Snapshot ts = transcription_service::GetSnapshot();
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_pending_recording_id.empty() || s_pending_recording_id == recording_id) {
        s_snapshot.phase = Phase::kFailed;
        s_snapshot.request_in_flight = false;
        s_snapshot.last_status_message =
            ts.last_status_message.empty() ? "Transcription unavailable" : ts.last_status_message;
        s_snapshot.last_error_code = ts.last_error_code;
        s_snapshot.last_error_message = ts.last_error_message;
        s_pending_recording_id.clear();
    }
    NotifyLocked();
    return false;
}

bool HandlePowerPressDown(const Context& context)
{
    if (Init() != ESP_OK) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        // No new take while the previous one is still being cued, replayed, or resolved.
        if (s_snapshot.phase == Phase::kStartCue || s_snapshot.phase == Phase::kStopCue ||
            s_snapshot.phase == Phase::kPlayingBack ||
            s_snapshot.phase == Phase::kAwaitingTagSelection ||
            s_snapshot.phase == Phase::kTranscribing) {
            return false;
        }

        const BlockedReason blocked_reason = EvaluateBlockedReason(context);
        if (blocked_reason != BlockedReason::kNone) {
            MarkBlockedLocked(blocked_reason);
            NotifyLocked();
            return false;
        }
    }

    const esp_err_t err = recording_service::Arm();
    std::lock_guard<std::mutex> lock(s_mutex);
    if (err != ESP_OK) {
        s_snapshot.phase = Phase::kFailed;
        s_snapshot.allowed = false;
        s_snapshot.blocked_reason = BlockedReason::kRecorderUnavailable;
        s_snapshot.last_status_message = "Recorder unavailable";
        s_snapshot.last_error_code = "record_arm_failed";
        s_snapshot.last_error_message = esp_err_to_name(err);
        NotifyLocked();
        return false;
    }

    ResetToIdleLocked();
    s_snapshot.phase = Phase::kArmed;
    s_snapshot.allowed = true;
    s_snapshot.last_status_message = kArmedStatus;
    NotifyLocked();
    return true;
}

bool HandlePowerLongPressStart(const Context& context)
{
    if (Init() != ESP_OK) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        const BlockedReason blocked_reason = EvaluateBlockedReason(context);
        if (blocked_reason != BlockedReason::kNone) {
            MarkBlockedLocked(blocked_reason);
            NotifyLocked();
            return false;
        }
        if (s_snapshot.phase != Phase::kArmed) {
            return false;
        }
    }

    // Capture starts before the cue, not after it: waiting for the cue to finish would
    // swallow the first word. The cue overlaps the opening moments of the take.
    const esp_err_t err = recording_service::Start(recording_service::StartMode::kFresh);
    uint32_t cue_token = 0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (err != ESP_OK) {
            s_snapshot.phase = Phase::kFailed;
            s_snapshot.last_status_message = "Recording failed to start";
            s_snapshot.last_error_code = "record_start_failed";
            s_snapshot.last_error_message = esp_err_to_name(err);
            NotifyLocked();
            return false;
        }

        cue_token = ++s_cue_token;
        s_snapshot.phase = Phase::kStartCue;
        s_snapshot.last_status_message = kStartCueStatus;
        s_snapshot.last_error_code.clear();
        s_snapshot.last_error_message.clear();
        NotifyLocked();
    }

    SystemSoundService::GetInstance().PlayCue(
        SoundCue::kSpeaking, [cue_token](SoundCuePlaybackResult result) {
            HandleStartCueResult(cue_token, result);
        });
    return true;
}

bool HandlePowerPressUp(const Context&)
{
    if (Init() != ESP_OK) {
        return false;
    }

    Phase phase = Phase::kIdle;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        phase = s_snapshot.phase;
    }

    if (phase == Phase::kArmed) {
        (void)recording_service::Cancel();
        std::lock_guard<std::mutex> lock(s_mutex);
        ResetToIdleLocked();
        NotifyLocked();
        return true;
    }

    // Released while the start cue is still playing: capture is already running, but the
    // cue owns the transition out of kStartCue. Defer the finish rather than dropping it,
    // otherwise a hold barely longer than the cue would record forever.
    if (phase == Phase::kStartCue) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_finish_pending_after_start_cue = true;
        return true;
    }

    if (phase != Phase::kRecording) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kSaving;
        s_snapshot.last_status_message = "Preparing recording";
        NotifyLocked();
    }
    (void)recording_service::Finish();
    return true;
}

bool SubmitTagSelection(int selected_index)
{
    if (Init() != ESP_OK) {
        return false;
    }

    if (selected_index < 0 || selected_index >= static_cast<int>(kTagOptions.size())) {
        return false;
    }

    ESP_LOGI(kTag,
             "Recording tag selection submitted: index=%d label=%.*s",
             selected_index,
             static_cast<int>(kTagOptions[static_cast<size_t>(selected_index)].label_text.size()),
             kTagOptions[static_cast<size_t>(selected_index)].label_text.data());

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_snapshot.phase != Phase::kAwaitingTagSelection) {
            return false;
        }
    }

    if (kTagOptions[static_cast<size_t>(selected_index)].is_discard) {
        recording_service::DiscardClip();
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kComplete;
        s_snapshot.has_clip = false;
        s_snapshot.clip_saved = false;
        s_snapshot.transcript_saved = false;
        s_snapshot.last_status_message = kDiscardedStatus;
        s_snapshot.last_error_code.clear();
        s_snapshot.last_error_message.clear();
        NotifyLocked();
        return true;
    }

    recording_service::RecordedClipPtr clip = recording_service::GetRecordedClip();
    if (!clip || clip->empty()) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kFailed;
        s_snapshot.last_status_message = "Save failed";
        s_snapshot.last_error_code = "recording_missing";
        s_snapshot.last_error_message = "Recording clip was not available";
        NotifyLocked();
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kSaving;
        s_snapshot.last_status_message = kSavingStatus;
        s_snapshot.last_error_code.clear();
        s_snapshot.last_error_message.clear();
        NotifyLocked();
    }

    recording_archive_service::SaveOptions options = {};
    options.tag = kTagOptions[static_cast<size_t>(selected_index)].tag;
    ESP_LOGI(kTag,
             "Starting archive save: tag=%s samples=%u duration_ms=%lu",
             recording_archive_service::TagName(options.tag),
             static_cast<unsigned>(clip->sample_count()),
             static_cast<unsigned long>(clip->duration_ms()));
    const recording_archive_service::SaveResult save_result =
        recording_archive_service::SaveClip(*clip, options);
    ESP_LOGI(kTag,
             "Archive save result: success=%d clip_saved=%d metadata_saved=%d id=%s wav=%s metadata=%s error=%s",
             save_result.success ? 1 : 0,
             save_result.clip_saved ? 1 : 0,
             save_result.metadata_saved ? 1 : 0,
             save_result.recording_id.empty() ? "<none>" : save_result.recording_id.c_str(),
             save_result.recording_path.empty() ? "<none>" : save_result.recording_path.c_str(),
             save_result.metadata_path.empty() ? "<none>" : save_result.metadata_path.c_str(),
             save_result.error_code.empty() ? "<none>" : save_result.error_code.c_str());

    const bool transcribe_endpoint_configured = transcription_service::GetSnapshot().provider_ready;
    const bool should_transcribe = save_result.clip_saved && transcribe_endpoint_configured;
    ESP_LOGI(kTag,
             "Transcription decision: clip_saved=%d transcribe_endpoint_configured=%d should_transcribe=%d",
             save_result.clip_saved ? 1 : 0,
             transcribe_endpoint_configured ? 1 : 0,
             should_transcribe ? 1 : 0);

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.clip_saved = save_result.clip_saved;
        s_snapshot.transcript_saved = false;
        s_snapshot.last_saved_recording_id = save_result.recording_id;
        s_snapshot.last_saved_recording_path = save_result.recording_path;
        s_snapshot.last_saved_transcript_path = save_result.transcript_path;
        s_pending_recording_id = save_result.recording_id;
    }

    if (should_transcribe && transcription_service::BeginTranscription(clip)) {
        recording_service::DiscardClip();
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.phase = Phase::kTranscribing;
        s_snapshot.request_in_flight = true;
        s_snapshot.last_status_message = kTranscribingStatus;
        s_snapshot.last_error_code.clear();
        s_snapshot.last_error_message.clear();
        NotifyLocked();
        return true;
    }

    if (should_transcribe) {
        ESP_LOGW(kTag,
                 "Transcription did not start after save: id=%s",
                 save_result.recording_id.empty() ? "<none>" : save_result.recording_id.c_str());
    }

    recording_service::DiscardClip();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.request_in_flight = false;
        s_snapshot.phase = save_result.clip_saved ? Phase::kComplete : Phase::kFailed;
        s_snapshot.last_status_message = save_result.clip_saved
                                             ? kSavedWithoutTranscriptStatus
                                             : (save_result.status_message.empty()
                                                    ? "Save failed"
                                                    : save_result.status_message);
        s_snapshot.last_error_code = save_result.error_code;
        s_snapshot.last_error_message = save_result.error_message;
        if (save_result.clip_saved && !save_result.error_code.empty()) {
            // Keep the saved result but surface the archive warning.
            s_snapshot.last_error_message = save_result.error_message;
        } else if (save_result.clip_saved) {
            s_snapshot.last_error_code.clear();
            s_snapshot.last_error_message.clear();
        }
        NotifyLocked();
    }
    return save_result.clip_saved;
}

void HandleRecordingEvent(const recording_service::Event& event)
{
    if (Init() != ESP_OK) {
        return;
    }

    recording_service::RecordedClipPtr clip = nullptr;
    bool should_show_tag_selection = false;
    GuardrailResult guardrail = {};
    bool discard_invalid_clip = false;
    bool queue_stop_cue = false;
    uint32_t cue_token = 0;

    if (event.state == recording_service::State::kClipReady && event.ui_state.has_clip) {
        clip = recording_service::GetRecordedClip();
        if (clip) {
            guardrail = ValidateClip(*clip, event.ui_state.duration_ms);
            should_show_tag_selection = guardrail.accepted;
        }
    }

    {
        std::lock_guard<std::mutex> lock(s_mutex);
        SyncRecordingStateLocked(event.ui_state);
        s_snapshot.request_in_flight = transcription_service::GetSnapshot().request_in_flight;

        if (event.state == recording_service::State::kArmed && s_snapshot.phase == Phase::kIdle) {
            s_snapshot.phase = Phase::kArmed;
            s_snapshot.last_status_message = kArmedStatus;
        } else if (event.state == recording_service::State::kRecording) {
            // Leave kStartCue alone -- HandleStartCueResult owns that transition, and
            // overwriting it here would drop the cue phase the moment capture engages.
            if (s_snapshot.phase != Phase::kStartCue) {
                s_snapshot.phase = Phase::kRecording;
                s_snapshot.last_status_message = kRecordingStatus;
            }
        } else if (event.state == recording_service::State::kClipReady) {
            if (should_show_tag_selection) {
                // Stop cue first, then playback, then the tag menu. HandleStopCueResult
                // drives the rest; queue_stop_cue defers the PlayCue call until the lock
                // is released so the callback cannot deadlock on a fast cue.
                cue_token = ++s_cue_token;
                queue_stop_cue = true;
                s_snapshot.phase = Phase::kStopCue;
                s_snapshot.last_status_message = kStopCueStatus;
                s_snapshot.last_error_code.clear();
                s_snapshot.last_error_message.clear();
            } else {
                discard_invalid_clip = true;
                s_snapshot.phase = Phase::kFailed;
                s_snapshot.has_clip = false;
                s_snapshot.last_status_message = guardrail.status_message;
                s_snapshot.last_error_code = guardrail.error_code;
                s_snapshot.last_error_message = guardrail.status_message;
            }
        }

        NotifyLocked();
    }
    if (discard_invalid_clip) {
        recording_service::DiscardClip();
    }
    if (queue_stop_cue) {
        SystemSoundService::GetInstance().PlayCue(
            SoundCue::kInterrupt, [cue_token](SoundCuePlaybackResult result) {
                HandleStopCueResult(cue_token, result);
            });
    }
}

void HandleTranscriptionEvent(const transcription_service::Event& event)
{
    if (Init() != ESP_OK) {
        return;
    }

    bool should_attach_transcript = false;
    std::string pending_recording_id;
    std::string transcript_text;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.request_in_flight = event.snapshot.request_in_flight;
        if (s_snapshot.phase == Phase::kTranscribing && !event.snapshot.request_in_flight &&
            !event.snapshot.last_transcript.empty() && !s_pending_recording_id.empty()) {
            should_attach_transcript = true;
            pending_recording_id = s_pending_recording_id;
            transcript_text = event.snapshot.last_transcript;
        }
    }

    if (should_attach_transcript) {
        ESP_LOGI(kTag,
                 "Attaching transcript to saved recording: id=%s chars=%u",
                 pending_recording_id.c_str(),
                 static_cast<unsigned>(transcript_text.size()));
        const recording_archive_service::SaveResult save_result =
            recording_archive_service::SaveTranscript(pending_recording_id, transcript_text);
        ESP_LOGI(kTag,
                 "Transcript save result: success=%d transcript_saved=%d metadata_saved=%d transcript=%s metadata=%s error=%s",
                 save_result.success ? 1 : 0,
                 save_result.transcript_saved ? 1 : 0,
                 save_result.metadata_saved ? 1 : 0,
                 save_result.transcript_path.empty() ? "<none>" : save_result.transcript_path.c_str(),
                 save_result.metadata_path.empty() ? "<none>" : save_result.metadata_path.c_str(),
                 save_result.error_code.empty() ? "<none>" : save_result.error_code.c_str());
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_snapshot.transcript_saved = save_result.transcript_saved;
            s_snapshot.last_saved_transcript_path = save_result.transcript_path;
            s_snapshot.last_transcript = transcript_text;
            s_snapshot.phase = Phase::kComplete;
            s_snapshot.request_in_flight = false;
            s_snapshot.last_status_message = save_result.transcript_saved
                                                 ? "Transcript saved to SD"
                                                 : kSavedWithoutTranscriptStatus;
            if (save_result.transcript_saved) {
                s_snapshot.last_error_code = save_result.error_code;
                s_snapshot.last_error_message = save_result.error_message;
            } else {
                s_snapshot.last_error_code = event.snapshot.last_error_code;
                s_snapshot.last_error_message = event.snapshot.last_error_message;
            }
            s_pending_recording_id.clear();
            NotifyLocked();
        }
        // This whole function can run while transcription_service::WorkerTask still holds its
        // own s_mutex (it calls the event handler chain from inside NotifyLocked()) -- calling
        // TryRetryOldestUnsentRecording() directly here would deadlock (see RequestRetryPass's
        // comment above). A successful save is our signal that the endpoint is reachable right
        // now, so kick an immediate retry pass instead of waiting out the full timer interval;
        // a failed save leaves it for the periodic timer (see kTranscriptionRetryIntervalUs).
        if (save_result.transcript_saved) {
            RequestRetryPass();
        }
        return;
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_snapshot.phase == Phase::kTranscribing && !event.snapshot.request_in_flight) {
        s_snapshot.phase = s_snapshot.clip_saved ? Phase::kComplete : Phase::kFailed;
        s_snapshot.last_status_message = s_snapshot.clip_saved
                                             ? kSavedWithoutTranscriptStatus
                                             : "Transcription failed";
        s_snapshot.last_error_code = event.snapshot.last_error_code;
        s_snapshot.last_error_message = event.snapshot.last_error_message;
        s_pending_recording_id.clear();
    }
    NotifyLocked();
}

const char* PhaseName(Phase phase)
{
    switch (phase) {
        case Phase::kArmed:
            return "armed";
        case Phase::kStartCue:
            return "start_cue";
        case Phase::kRecording:
            return "recording";
        case Phase::kStopCue:
            return "stop_cue";
        case Phase::kPlayingBack:
            return "playing_back";
        case Phase::kAwaitingTagSelection:
            return "awaiting_tag_selection";
        case Phase::kSaving:
            return "saving";
        case Phase::kTranscribing:
            return "transcribing";
        case Phase::kComplete:
            return "complete";
        case Phase::kFailed:
            return "failed";
        case Phase::kIdle:
        default:
            return "idle";
    }
}

}  // namespace recording_session_service
