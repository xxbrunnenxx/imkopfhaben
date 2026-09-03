# LVGL + ESP-IDF Referenz

## Versions-Kompatibilitätsmatrix

| LVGL | Minimum ESP-IDF | Hinweise |
|---|---|---|
| v8.3.x | 4.4+ | Stabil, weit verbreitet; nutzt `lv_disp_drv_t` / `lv_indev_drv_t` API |
| v9.0.x | 5.0+ | Breaking API-Änderung gegenüber v8 — `lv_display_t`, neue Flush-Callback-Signatur |
| v9.1+ | 5.1+ | Empfohlen für neue Projekte auf IDF 5.x |

**Vor jedem Treiber- oder Integrationscode immer die exakte LVGL-Version in `idf_component.yml` oder `CMakeLists.txt` bestätigen. Die v8→v9-API-Änderung ist nicht rückwärtskompatibel.**

LVGL über den IDF Component Manager beziehen:
```yaml
# idf_component.yml
dependencies:
  lvgl/lvgl: "^9.1.0"
  # or for v8:
  # lvgl/lvgl: "^8.3.0"
```

Oder über Managed Components:
```bash
idf.py add-dependency "lvgl/lvgl^9.1.0"
```

---

## Display-Flush-Callback (v9.x)

```c
#include "lvgl.h"
#include "driver/spi_master.h"

static spi_device_handle_t spi;

// Called by LVGL when a render area is ready to be sent to the display.
// Must call lv_display_flush_ready() when transfer is complete.
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    // Set the display window (controller-specific — example: ILI9341/ST7789 sequence)
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    // DMA SPI transfer — px_map must be in DMA-capable memory
    spi_transaction_t t = {
        .length = w * h * 2 * 8,  // bits; 2 bytes per pixel for RGB565
        .tx_buffer = px_map,
        .flags = 0,
    };
    spi_device_queue_trans(spi, &t, portMAX_DELAY);

    // Signal LVGL that flush is done.
    // If using DMA with a callback, call this from the DMA completion ISR instead:
    lv_display_flush_ready(disp);
}
```

**v8.x-Äquivalent (andere Funktionssignatur):**
```c
static void disp_flush_v8(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    // ... transfer logic ...
    lv_disp_flush_ready(drv);  // note: lv_disp_flush_ready, not lv_display_flush_ready
}
```

---

## Display-Initialisierung

### v9.x
```c
#include "lvgl.h"

#define DISP_WIDTH   320
#define DISP_HEIGHT  240
#define BUF_LINES    20   // number of lines in each draw buffer

static lv_display_t *disp;
static lv_color_t buf1[DISP_WIDTH * BUF_LINES];
static lv_color_t buf2[DISP_WIDTH * BUF_LINES];  // optional second buffer for double-buffering

void lvgl_display_init(void)
{
    lv_init();

    disp = lv_display_create(DISP_WIDTH, DISP_HEIGHT);
    lv_display_set_flush_cb(disp, disp_flush);

    // Single buffer:
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Double buffer (smoother rendering, uses 2x RAM):
    // lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Full-screen buffer in PSRAM (ESP32-S3 with PSRAM):
    // lv_color_t *fb = heap_caps_malloc(DISP_WIDTH * DISP_HEIGHT * sizeof(lv_color_t),
    //                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // lv_display_set_buffers(disp, fb, NULL, DISP_WIDTH * DISP_HEIGHT * sizeof(lv_color_t),
    //                        LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
}
```

---

## Tick-Quelle (erforderlich)

LVGL braucht einen Millisekunden-Tick, um zu animieren, Events zu timen und Übergänge zu steuern.

### Option A: FreeRTOS-Timer (bevorzugt)
```c
static void lvgl_tick_timer_cb(TimerHandle_t xTimer)
{
    lv_tick_inc(portTICK_PERIOD_MS);  // usually 1ms if configTICK_RATE_HZ=1000
}

void lvgl_tick_init(void)
{
    TimerHandle_t timer = xTimerCreate("lvgl_tick", pdMS_TO_TICKS(1),
                                       pdTRUE, NULL, lvgl_tick_timer_cb);
    xTimerStart(timer, 0);
}
```

### Option B: esp_timer (höhere Auflösung)
```c
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(1);  // called every 1ms
}

void lvgl_tick_init(void)
{
    const esp_timer_create_args_t args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000));  // 1000µs = 1ms
}
```

---

## LVGL-Task und Mutex (Thread-Sicherheit)

LVGL ist **nicht thread-sicher**. Alle `lv_`-Aufrufe — inklusive UI-Aufbau, Style-Updates und Animationen — müssen aus demselben Task kommen, der `lv_timer_handler()` aufruft, oder durch ein Mutex geschützt sein.

### Muster: eigener LVGL-Task
```c
static SemaphoreHandle_t lvgl_mutex;

void lvgl_lock(void)   { xSemaphoreTakeRecursive(lvgl_mutex, portMAX_DELAY); }
void lvgl_unlock(void) { xSemaphoreGiveRecursive(lvgl_mutex); }

static void lvgl_task(void *arg)
{
    lvgl_tick_init();
    lvgl_display_init();
    ui_init();  // create screens, widgets, etc.

    while (true) {
        lvgl_lock();
        uint32_t time_to_next = lv_timer_handler();
        lvgl_unlock();
        vTaskDelay(pdMS_TO_TICKS(time_to_next > 5 ? 5 : time_to_next));
    }
}

void app_main(void)
{
    lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 5, NULL,
                            1);  // pin to core 1; leave core 0 for Wi-Fi/BLE
}

// Updating UI from another task:
void update_label_from_task(lv_obj_t *label, const char *text)
{
    lvgl_lock();
    lv_label_set_text(label, text);
    lvgl_unlock();
}
```

---

## Farbformat und Byte-Reihenfolge

**Das ist die häufigste Ursache für falsche Farben und ausgewaschene Display-Ausgabe.**

### Das erwartete Format des eigenen Controllers bestimmen
| Controller | Typisches Format | Byte-Reihenfolge |
|---|---|---|
| ILI9341 | RGB565 | Big-Endian (MSB zuerst) |
| ST7789 | RGB565 | Big-Endian |
| SH8601 | RGB888 oder ARGB8888 | Abhängig von der Initialisierung |
| GC9A01 | RGB565 | Big-Endian |
| RA8875 | RGB565 | Big-Endian |

### LVGL-Farbformat konfigurieren
```c
// v9.x — set on the display object:
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
// or
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);
```

### Byte-Swap für SPI-Controller
Die meisten ESP32-SPI-Controller senden standardmäßig LSB-first; die meisten Display-Controller erwarten Big-Endian-RGB565. Beheben mit:
```c
// v9.x:
lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
// Enable byte swap in the display object (swaps bytes of each 16-bit pixel before flush):
// lv_display_set_byte_swap(disp, true);   // available in v9.1+

// Or: swap in hardware via SPI controller flag:
// .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY  -- check your IDF version
```

Sind die Farben invertiert (Blau erscheint als Rot), ist die Byte-Reihenfolge falsch. Sehen die Farben korrekt, aber ausgewaschen/dunkel aus, stimmt der Alpha-Kanal oder die Bittiefe nicht.

---

## Speicher-Konfiguration

### sdkconfig-Optionen
```
# Increase task stack for LVGL rendering (default is often too small):
# The lvgl task itself: 8192–16384 bytes depending on widget complexity.

# Enable PSRAM for large frame buffers (ESP32-S3):
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y          # ESP32-S3 Octal PSRAM
CONFIG_SPIRAM_SPEED_80M=y

# Allow malloc from PSRAM:
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384  # keep small allocs in IRAM
```

### Draw-Buffer zuweisen
```c
// Internal SRAM (fast, limited — use for small buffers or when PSRAM unavailable):
static lv_color_t buf[LCD_WIDTH * 20];  // 20 lines

// PSRAM (ESP32-S3 — for large/full-frame buffers):
lv_color_t *buf = heap_caps_aligned_alloc(64,
    LCD_WIDTH * LCD_HEIGHT * sizeof(lv_color_t),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
assert(buf != NULL);

// DMA-capable (required for SPI DMA transfers — must NOT be in PSRAM on some targets):
lv_color_t *dma_buf = heap_caps_aligned_alloc(64,
    LCD_WIDTH * 20 * sizeof(lv_color_t),
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
```

**Auf dem ESP32 (original):** DMA-fähiger Speicher ist IRAM/DRAM; PSRAM ist für SPI nicht DMA-fähig.
**Auf dem ESP32-S3:** PSRAM kann für SPI-DMA mit EDMA genutzt werden (Treiber-Doku und `MALLOC_CAP_DMA` prüfen).

---

## Performance-Tuning

### Double Buffering
Zwei Render-Buffer nutzen, damit LVGL den nächsten Frame vorbereiten kann, während die DMA den aktuellen überträgt:
```c
lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
```
Im Flush-Callback die DMA starten und sofort zurückkehren. `lv_display_flush_ready()` aus dem DMA-Completion-Callback aufrufen — das erlaubt LVGL, das Rendern des nächsten Frames gleichzeitig zu beginnen.

### Blockieren im Flush-Callback vermeiden
```c
// Bad: blocks until transfer completes
spi_device_transmit(spi, &t);
lv_display_flush_ready(disp);

// Better: queue DMA, signal completion from ISR or polling callback
spi_device_queue_trans(spi, &t, portMAX_DELAY);
// lv_display_flush_ready() called from DMA complete CB
```

### SPI-Frequenz
- ILI9341/ST7789: typischerweise 40–80MHz, abhängig von der Board-Leiterbahnqualität
- Bei 20MHz starten, erhöhen bis Artefakte auftreten, dann 10% zurückgehen
- Einstellen über `spi_device_interface_config_t.clock_speed_hz`

### Dirty-Region-Rendering
LVGL zeichnet nur geänderte Bereiche neu. `lv_obj_invalidate()` nicht unnötig auf ganzen Bildschirmen aufrufen. Lieber einzelne Labels, Arcs oder Bilder aktualisieren.

---

## Häufige Fallstricke

| Symptom | Ursache | Behebung |
|---|---|---|
| Bildschirm komplett weiß/schwarz nach Init | Flush-Callback nie aufgerufen oder Controller nicht initialisiert | Prüfen, ob `lv_timer_handler()` aufgerufen wird; Display-Init-Sequenz prüfen |
| Farben falsch (Blau ↔ Rot) | RGB-Byte-Reihenfolge falsch | Byte-Swap aktivieren oder R/B im Flush-Callback tauschen |
| Farben ausgewaschen / dunkel | Falsche Farbtiefe (z. B. 24-Bit-Daten an einen 16-Bit-Controller) | `lv_display_set_color_format()` an den Controller anpassen |
| Absturz im Flush-Callback | Draw-Buffer nicht in DMA-fähigem Speicher | `MALLOC_CAP_DMA\|MALLOC_CAP_INTERNAL` für SPI-DMA-Buffer nutzen |
| Flackern / Tearing | Einzelner Buffer, kein Vsync | Double Buffer nutzen; DMA-Completion-Signalisierung ergänzen |
| UI hängt nach ein paar Updates | `lv_timer_handler()` blockiert oder Mutex-Deadlock | Sicherstellen, dass der LVGL-Task nicht blockiert; Mutex-Acquire/Release-Paarung prüfen |
| `lv_tick_inc` wird nicht aufgerufen | Keine Tick-Quelle konfiguriert | FreeRTOS-Timer oder `esp_timer` ergänzen, der alle 1ms `lv_tick_inc(1)` aufruft |
| Animationen stottern | `lv_timer_handler()` wird zu selten aufgerufen | Sleep auf 5ms deckeln; `vTaskDelay(time_to_next)` nicht mit großen Werten aufrufen |
| Stack-Overflow im LVGL-Task | Komplexe Widgets überschreiten den Task-Stack | Task-Stack für komplexe UIs auf 16384+ Bytes erhöhen |

---

## LVGL + ESP-IDF Component-Manager-Lock-Datei

Nach dem Ermitteln einer funktionierenden Kombination in der Kompatibilitäts-Lock-Datei des Projekts festhalten:

```yaml
# esp-framework-compat.lock
esp-idf: "v5.2.1"
lvgl: "v9.1.0"   # from idf_component.yml / managed_components
display-controller: "ST7789"
notes: "RGB565, big-endian, 40MHz SPI, double-buffer DMA on ESP32-S3 with Octal PSRAM"
verified: "2025-01-15"
```
