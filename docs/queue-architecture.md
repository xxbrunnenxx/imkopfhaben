# Queue & Wartelisten-Architektur

Dieses Dokument beschreibt, wie Transkriptions- und Zusammenfassungs-Anfragen in Followup gepuffert und verarbeitet werden.

## Überblick

Die AI-Services (`transcription_service` und `summary_service`) müssen Anfragen verarbeiten, die über HTTP-Netzwerk zum lokalen LM Studio Server gehen. Um Netzwerk-Timeouts, Überlastung und Stack-Überläufe zu vermeiden, nutzen beide Services **Wartelisten** – allerdings mit unterschiedlichen Strategien.

```
User Recording
     │
     ├─► recording_session_service
     │   (orchestriert Aufnahme/Review/Tag/Save)
     │
     ├─► [Optional] transcription_service
     │   ├─ Task-per-Request Model
     │   └─ Serialisiert durch session_service
     │
     └─► [Optional] summary_service
         ├─ FreeRTOS Queue (Depth=4)
         └─ 1 Worker Task
```

---

## 1. summary_service: FreeRTOS Queue Model

### Architektur

```cpp
constexpr UBaseType_t kSummaryQueueDepth = 4;
QueueHandle_t s_queue = nullptr;

struct QueuedRequest {
    SummaryKind kind = SummaryKind::kNone;  // kNotes oder kTodos
};

std::mutex s_mutex;
Snapshot s_snapshot = {};  // Global state
```

### Initialization (Init)

```cpp
esp_err_t Init() {
    std::lock_guard<std::mutex> lock(s_mutex);
    
    if (s_snapshot.initialized) {
        return ESP_OK;
    }
    
    // FreeRTOS Queue erstellen
    s_queue = xQueueCreate(kSummaryQueueDepth, sizeof(QueuedRequest));
    if (s_queue == nullptr) {
        ESP_LOGE(kTag, "Failed to create summary queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Worker Task starten
    if (xTaskCreatePinnedToCore(
        WorkerTask,
        "summary_service",
        kWorkerTaskStackWords,  // 8192 words
        nullptr,
        followup_task_config::kPriorityLocalAi,
        nullptr,
        followup_task_config::kSystemCore) != pdPASS) {
        ESP_LOGE(kTag, "Failed to start summary worker");
        vQueueDelete(s_queue);
        s_queue = nullptr;
        return ESP_ERR_NO_MEM;
    }
    
    s_snapshot.initialized = true;
    return ESP_OK;
}
```

### Request einqueuen

```cpp
bool RequestSummary(SummaryKind kind) {
    if (kind != SummaryKind::kNotes && kind != SummaryKind::kTodos) {
        return false;  // Invalid kind
    }
    
    QueueHandle_t queue = nullptr;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        
        // Preconditions checken
        if (!s_snapshot.initialized || s_queue == nullptr) {
            return false;  // Not initialized
        }
        if (s_snapshot.request.in_flight) {
            return false;  // Already processing a request
        }
        
        // State updaten
        s_snapshot.request.in_flight = true;
        s_snapshot.request.kind = kind;
        s_snapshot.request.phase = RequestPhase::kStarted;
        s_snapshot.request.status_message = std::string("Summarizing ") + 
                                             SegmentLabelForKind(kind);
        s_snapshot.request.error_code.clear();
        s_snapshot.request.error_message.clear();
        ++s_snapshot.request_generation;  // Stale result detection
        
        queue = s_queue;
        NotifyLocked();
    }
    
    // Außerhalb des Locks queuen
    QueuedRequest request = {.kind = kind};
    if (xQueueSend(queue, &request, 0) == pdTRUE) {
        return true;  // Success
    }
    
    // Queue voll oder Fehler
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_snapshot.request.in_flight = false;
        s_snapshot.request.phase = RequestPhase::kFailed;
        s_snapshot.request.status_message = "Unable to queue summary request";
        s_snapshot.request.error_code = "queue_full";
        s_snapshot.request.error_message = "Unable to queue summary request";
        ++s_snapshot.request_generation;
        NotifyLocked();
    }
    return false;
}
```

### Worker Task (Verarbeitung)

```cpp
void WorkerTask(void*) {
    while (true) {
        QueuedRequest request = {};
        
        // Auf Request warten (blockierend)
        if (s_queue == nullptr || 
            xQueueReceive(s_queue, &request, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        
        // Summary generieren & speichern
        GenerationResult result = GenerateSummary(request.kind);
        
        if (result.success && !PersistSummary(request.kind, result)) {
            result.success = false;
            result.error_code = "summary_save_failed";
            result.error_message = "Unable to save summary to SD card";
        }
        
        CompleteSummaryRequest(request.kind, result);
    }
}
```

### State Machine (Phasen)

```
          ┌────────────────┐
          │     kIdle      │
          └────────┬───────┘
                   │
          RequestSummary()
                   │
          ┌────────▼────────┐
          │  kStarted       │ ← "Summarizing Notes..." (in UI)
          └────────┬────────┘
                   │
          (Worker verarbeitet)
                   │
          ┌────────▼──────────┐
          │  kSucceeded       │ ← "Notes summary updated"
          │  oder             │    oder
          │  kFailed          │    "Unable to summarize..."
          └───────────────────┘
```

### Fehlerbehandlung

| Fehler | Error Code | Bedeutung |
|--------|-----------|-----------|
| `queue_full` | `queue_full` | Max. 4 Requests bereits queued |
| `local_ai_not_ready` | `local_ai_not_ready` | Lokaler AI Server nicht erreichbar |
| `local_ai_not_configured` | `local_ai_not_configured` | Keine LM Studio URL konfiguriert |
| `storage_read_failed` | `storage_read_failed` | SD Zugriff fehlgeschlagen |
| `no_summary_source` | `no_summary_source` | Keine transkribierten Aufnahmen vorhanden |
| `summary_failed` | `summary_failed` | LM Studio HTTP Request fehlgeschlagen |
| `summary_save_failed` | `summary_save_failed` | Summary konnte nicht auf SD gespeichert werden |

---

## 2. transcription_service: Task-per-Request Model

Im Gegensatz zu `summary_service` hat `transcription_service` **keine zentrale Queue**, sondern erstellt für **jeden Request** eine neue FreeRTOS Task.

### Architektur

```cpp
struct TaskContext {
    recording_service::RecordedClipPtr clip = {};  // Audio-Daten
};

std::mutex s_mutex;
bool s_request_in_flight = false;
std::string s_last_transcript = {};
```

### Request einqueuen

```cpp
bool BeginTranscription(recording_service::RecordedClipPtr clip) {
    if (!clip || clip->empty()) {
        return false;  // No audio
    }
    
    auto* task_context = new TaskContext();
    task_context->clip = std::move(clip);
    
    TaskHandle_t task = nullptr;
    const BaseType_t created = xTaskCreatePinnedToCore(
        WorkerTask,
        "transcription",
        kWorkerTaskStackWords,  // 8192 words
        task_context,
        followup_task_config::kPriorityLocalAi,
        &task,
        followup_task_config::kSystemCore);
    
    if (created != pdPASS) {
        delete task_context;
        
        std::lock_guard<std::mutex> lock(s_mutex);
        s_request_in_flight = false;
        s_last_status_message = "Transcription unavailable";
        s_last_error_code = "task_start_failed";
        s_last_error_message = "Failed to queue transcription task";
        NotifyLocked();
        
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_request_in_flight = true;
        s_last_status_message = "Transcribing...";
        s_last_error_code.clear();
        NotifyLocked();
    }
    
    return true;
}
```

### Worker Task

```cpp
void WorkerTask(void* raw_context) {
    std::unique_ptr<TaskContext> context(static_cast<TaskContext*>(raw_context));
    
    if (!context || !context->clip || context->clip->empty()) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_request_in_flight = false;
        s_last_error_code = "empty_audio";
        NotifyLocked();
        vTaskDelete(nullptr);
        return;
    }
    
    // HTTP Request zum LM Studio Server
    const local_ai_service::TranscriptionResult result = 
        local_ai_service::Transcribe(*context->clip);
    
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_request_in_flight = false;
        
        if (result.success) {
            s_last_status_message = "Transcript ready";
            s_last_error_code.clear();
            s_last_error_message.clear();
            s_last_transcript = result.transcript;
        } else {
            s_last_status_message = "Transcription failed";
            s_last_error_code = result.error_code;
            s_last_error_message = result.error_message;
            s_last_transcript.clear();
        }
        
        NotifyLocked();
    }
    
    vTaskDelete(nullptr);  // Task löscht sich selbst
}
```

### Serialisierung durch recording_session_service

Die echte Serialisierung kommt nicht von `transcription_service` selbst, sondern von `recording_session_service`, der die gesamte Lifecycle orchestriert:

```
recording_session_service::Phase

kArmed
  ↓
kStartCue → (beep)
  ↓
kRecording → (Audio in PSRAM)
  ↓
kStopCue → (beep)
  ↓
kPlayingBack → BeginArchivedTranscription() [später re-transkribierende]
  ↓            oder direkt aus BeginTranscription() [neue Aufnahme]
kAwaitingTagSelection
  ↓
kSaving → (WAV auf SD)
  ↓
kTranscribing → transcription_service::BeginTranscription()
  │             └─ Neue Task erstellt & läuft
  │
kComplete
```

### Fehlerbehandlung

| Fehler | Error Code | Bedeutung |
|--------|-----------|-----------|
| `empty_audio` | `empty_audio` | Keine Audio-Daten |
| `task_start_failed` | `task_start_failed` | Task konnte nicht erstellt werden (Speicher?) |
| `network_unreachable` | `network_unreachable` | WiFi nicht verbunden |
| `http_timeout` | `http_timeout` | LM Studio antwortet nicht |
| `http_error` | `http_error` + HTTP Code | Server gab Fehler zurück |

---

## 3. Vergleich: Queue vs. Task-per-Request

| Aspekt | summary_service | transcription_service |
|--------|-----------------|----------------------|
| **Queue Mechanismus** | FreeRTOS Queue (xQueueCreate) | Keine Queue |
| **Queue Tiefe** | 4 Requests max. | N/A |
| **Worker Model** | 1 dedizierter Worker Task | Task-per-Request |
| **Parallelität** | Seriell (Queue) | Parallel möglich |
| **Serialisierung** | Automatisch durch Queue | Durch `recording_session_service` |
| **Speicher-Risiko** | Gering (1 Worker) | Höher (viele Tasks) |
| **Stack pro Request** | 8192 words (1 Task) | 8192 words × N Tasks |
| **Error bei Überlast** | `queue_full` | `task_start_failed` |

---

## 4. Wartelisten-Strategien: Best Practices

### ✅ summary_service Pattern (Queue-basiert)

**Wann nutzen:**
- Anfragen müssen seriell verarbeitet werden
- Netzwerk ist der Bottleneck
- Memory ist begrenzt (keine neuen Tasks für jeden Request)
- UI sollte Anfragen puffern können

**Vorteile:**
- Vorhersagbarer Speicherverbrauch
- Einfache Fehlerbehandlung
- Gute Kontrolle über Parallelität

**Nachteile:**
- Max 4 Requests
- Später eingequeute Requests müssen warten

### ✅ transcription_service Pattern (Task-per-Request)

**Wann nutzen:**
- Anfragen sind unabhängig & können parallel laufen
- Stack-Speicher ist reichlich vorhanden
- Wenige parallele Requests erwartet
- Schnelle Response nötig

**Vorteile:**
- Keine Warteschlange-Overhead
- Anfragen können parallel laufen
- Flexibel

**Nachteile:**
- Höherer Speicherverbrauch (viele Tasks)
- Schwächer bei plötzlichen Anfrage-Bursts
- Harder zu debuggen (viele Tasks)

---

## 5. Zukünftige Verbesserungen

### Möglichkeit 1: transcription_service auch mit Queue
Wenn mehrere parallele Transkriptionen zu Speicherproblemen führen, könnten wir `transcription_service` auf das Queue-Modell portieren (ähnlich wie `summary_service`).

```cpp
// TODO: Implement queued transcription
constexpr UBaseType_t kTranscriptionQueueDepth = 2;  // Max 2 parallel
QueueHandle_t s_transcription_queue = nullptr;
```

### Möglichkeit 2: Gemeinsame Queue für beide Services
Wenn beide Services denselben LM Studio Server nutzen und Bandbreite sparen wollen, könnten sie eine **gemeinsame Netzwerk-Request-Queue** teilen.

```cpp
// Hypothetisches Modell:
// local_ai_service::QueueRequest(clip, kind, callback)
// ├─ Serialisiert HTTP Requests
// └─ Callbacks triggern transcription/summary completion
```

### Möglichkeit 3: Priority Queue
Wenn wichtige Requests Vorrang haben sollen (z.B. aufgezeichnete Transkription > Zusammenfassungs-Hintergrund):

```cpp
constexpr UBaseType_t kQueueDepth = 8;
// Nutze xQueueSendToFrontFromISR() für High-Priority Requests
```

---

## 6. Monitoring & Debugging

### Event Handler Pattern

Beide Services nutzen das gleiche Observer-Pattern:

```cpp
using EventHandler = void (*)(const Event& event, void* context);

void summary_service::SetEventHandler(EventHandler handler, void* context);
void transcription_service::SetEventHandler(EventHandler handler, void* context);
```

In `app_shell::HandleSummaryEvent()` und `app_shell::HandleTranscriptionEvent()`:

```cpp
void HandleSummaryEvent(const summary_service::Event& event, void*) {
    ESP_LOGI(kTag,
        "Summary intent: ready=%d in_flight=%d phase=%s error=%s",
        event.snapshot.provider_ready ? 1 : 0,
        event.snapshot.request.in_flight ? 1 : 0,
        RequestPhaseName(event.snapshot.request.phase),
        event.snapshot.request.error_code.empty() ? "<none>" 
                                                   : event.snapshot.request.error_code.c_str());
}
```

### Logs

Nutze `ESP_LOGI()` / `ESP_LOGW()` zum Debuggen:

```bash
# Alle Summary/Transcription Logs anschauen:
idf.py -p /dev/ttyUSB0 monitor | grep -E "(SummaryService|TranscriptionSvc)"
```

---

## 7. Zusammenfassung

| Service | Queue-Tiefe | Worker Model | Status |
|---------|------------|--------------|--------|
| **summary_service** | 4 | 1 Worker Task | ✅ Queue-based, produktiv |
| **transcription_service** | ∞ | Task-per-Request | ⚠️ Könnte zu Queue portiert werden |

**Nächste Schritte:**
- [ ] Hardware-Testing mit vollen Workloads
- [ ] Ggf. `transcription_service` zu Queue portieren
- [ ] Monitoring für Queue-Fullness in UI
- [ ] Dokumentation für Benutzer (was bedeutet "queue_full"?)
