# ESP32-FreeRTOS-Patterns (ESP-IDF)

Nutze diese Referenz für ESP32/ESP-IDF-Task-Design, Synchronisation, ISR-Übergabe, Timer und Watchdog-sichere Nebenläufigkeits-Gestaltung.

## Umfang und Standardwerte

- Zielt auf ESP-IDF-Projekte mit der eingebauten FreeRTOS-Integration.
- Bevorzuge Standard-FreeRTOS-APIs plus ESP-IDF-Integrationen (`esp_event`, `esp_timer`, `esp_task_wdt`) gegenüber eigenen Schedulern.
- Behandle Dual-Core-Verhalten auf klassischem ESP32/ESP32-S3 als expliziten Design-Aspekt. Nicht bei allen ESP32-Varianten Dual-Core annehmen (z. B. ist ESP32-C3 Single-Core).

## Task-Design-Patterns

### Periodischer Task (ohne Drift)

- `vTaskDelayUntil()` für periodisches Sampling/Regel-Schleifen nutzen.
- Ausführungszeit messen und Verpasser loggen, falls die Schleife ihre Periode überschreiten kann.
- Peripherie-Transaktionen mit Timeouts begrenzt halten.

```c
static void sensor_task(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(100);

    for (;;) {
        read_sensor_once_with_timeout();
        vTaskDelayUntil(&last, period);
    }
}
```

### Event-getriebener Task

- Eine Queue bevorzugen, wenn Nutzdaten übertragen werden müssen.
- Task-Notifications bevorzugen, wenn nur Signalisierung/Bit-Flags nötig sind (geringerer Overhead als Semaphoren).
- Event-Gruppen für Bereitschafts-Gates über mehrere Subsysteme bevorzugen.

## ISR-zu-Task-Übergabe (ESP32-spezifische Einschränkungen)

- ISRs kurz und `IRAM`-sicher halten, falls sie laufen können, während der Flash-Cache nicht verfügbar ist.
- `IRAM_ATTR` bei zeitkritischen ISRs nutzen und sicherstellen, dass aufgerufene Funktionen/Daten wie vom Interrupt-Kontext gefordert in IRAM/DRAM liegen.
- Nur `xQueueSendFromISR()`, `xTaskNotifyFromISR()` oder `vTaskNotifyGiveFromISR()` nutzen.
- `portYIELD_FROM_ISR()` aufrufen, wenn ein Task mit höherer Priorität aufgeweckt wurde.
- Niemals blockierende ESP-IDF-Treiber-APIs aus einer ISR heraus aufrufen.

```c
static TaskHandle_t s_worker_task;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    BaseType_t hp_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_worker_task, &hp_task_woken);
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}
```

## Core-Affinität und Prioritäts-Leitfaden

- Tasks nicht auf einen Core festpinnen, außer es gibt einen klaren Grund (Latenz, Treiber-Affinität, Cache-Verhalten, Isolation).
- `xTaskCreatePinnedToCore()` nur nutzen, wenn gemessenes Verhalten es erfordert.
- Prioritäts-Inversionen prüfen, wenn mehrere Tasks sich I2C-/SPI-/UART-/Netzwerk-Ressourcen teilen.
- Lange blockierende Aufrufe in hochprioren Tasks vermeiden; sie lösen häufig Watchdog-Symptome aus.

## Synchronisations-Patterns

### Mutexe

- Mutexe für gemeinsam genutzte Peripherie-Busse (I2C/SPI) oder gemeinsamen Zustand mit nicht-trivialen kritischen Abschnitten nutzen.
- Sperrzeit kurz halten; während des Haltens eines Mutex nicht ausgiebig loggen.
- Einen einzigen Besitzer-Task für komplexe Peripherie bevorzugen, statt viele Tasks einen Mutex teilen zu lassen.

### Kritische Abschnitte

- Nur für sehr kurze, begrenzte Read-Modify-Write-Operationen nutzen.
- Treiber-Aufrufe, Logging oder Queue-Operationen nicht in kritischen Abschnitten einwickeln.
- Daran denken, dass kritische Abschnitte die Interrupt-Latenz und das Watchdog-Risiko erhöhen können.

## Timer: das richtige Werkzeug wählen

- `esp_timer`: hochauflösende Callbacks, verzögerte Arbeitsplanung, Mikrosekunden-Zeitbasis.
- FreeRTOS-Software-Timer: leichtgewichtige Timer-Callbacks im Task-Kontext, periodische Arbeit im Millisekunden-Maßstab.
- `gptimer`-Treiber: Hardware-Timer-Peripherie, für Wellenform-/Capture-/engere Timing-Anwendungsfälle.

Faustregel:
- Regel-Schleife/Task-Takt -> Task + `vTaskDelayUntil()`
- App-seitiger Retry/Backoff -> FreeRTOS-Timer oder `esp_timer`
- Präzises Peripherie-Timing/Capture -> Hardware-Timer (`gptimer`, RMT, LEDC je nach Anwendungsfall)

## ESP-IDF-Event-Loop-Integration

- `esp_event` für WLAN-, IP- und andere Subsystem-Events statt Ad-hoc-Polling nutzen.
- Event-Handler klein halten; schwere Arbeit an einen Task/eine Queue delegieren.
- Handler-Registrierung/-Deregistrierung nachverfolgen, um doppelte Callbacks und Lecks zu vermeiden.

## Speicher- und Stack-Überwachung (RTOS-fokussiert)

- Stack-Reserven während des Testens mit `uxTaskGetStackHighWaterMark()` überwachen.
- Heap-Gesundheit mit `heap_caps_get_free_size()` und `heap_caps_get_minimum_free_size()` beobachten.
- Task-Stack-Größe als Design-Parameter behandeln, besonders für Logging-, JSON/TLS- und Protokoll-Parsing-Pfade.

## Watchdog und Lebendigkeit

- `esp_task_wdt` für langlaufende Tasks in der Produktion nutzen, wo angebracht.
- Watchdogs bewusst füttern/überwachen; Watchdog-Resets nicht "beheben", indem man zuerst den Watchdog deaktiviert.
- Grundursachen untersuchen: Deadlocks, lange kritische Abschnitte, Busy-Loops, blockierte Callbacks oder ausgehungerte niedrigpriore Tasks.

## Laufzeit-Introspektion über Service-Terminal (empfohlen)

- Falls ein USB-/Seriell-Service-Terminal existiert, begrenzte RTOS-Diagnose-Kommandos anbieten:
  - Task-Liste/-Zustand/-Priorität
  - Stack-High-Water-Marks
  - Heap-/Min-Frei-Schnappschüsse
- Snapshot-artige Kommandos gegenüber langlaufenden Berichten bevorzugen.
- Sicherstellen, dass Diagnose-Kommandos keine kritischen Tasks blockieren oder gemeinsame Locks lange halten.

## Review-Checkliste (zusammengeführt und ESP32-angepasst)

- `vTaskDelayUntil()` für periodische Tasks nutzen, um Drift zu vermeiden.
- ISRs kurz halten und Arbeit über Queue/Notification/Event-Bits an Tasks delegieren.
- ISR-sichere API-Nutzung und IRAM-Sicherheit für ISR-Pfade prüfen.
- Task-Notifications nutzen, wenn keine Nutzdaten-Übertragung nötig ist.
- Stack-Größen aus Messung dimensionieren, nicht raten.
- Mutexe gegenüber langen kritischen Abschnitten bevorzugen; Prioritäts-Inversions-Risiko prüfen.
- Heap/Stack während Bring-up und Regressionstests überwachen.
- Watchdog-Strategie für Produktions-Builds bestätigen.
- Falls ein Service-Terminal existiert, prüfen, dass RTOS-Diagnose-Kommandos sicher und begrenzt sind.
