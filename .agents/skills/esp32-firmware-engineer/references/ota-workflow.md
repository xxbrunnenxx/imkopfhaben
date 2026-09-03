# OTA-Workflow-Referenz

## Anforderungen an das Partitions-Layout

### Minimales 2-OTA-Layout (4 MB Flash)
```csv
# Name,   Type, SubType, Offset,   Size,  Flags
nvs,      data, nvs,     0x9000,   0x6000,
otadata,  data, ota,     0xf000,   0x2000,
phy_init, data, phy,     0x11000,  0x1000,
ota_0,    app,  ota_0,   0x20000,  0x180000,
ota_1,    app,  ota_1,   0x1a0000, 0x180000,
```

### Factory + 2-OTA (für Rollback bevorzugt)
```csv
nvs,      data, nvs,     0x9000,   0x6000,
otadata,  data, ota,     0xf000,   0x2000,
phy_init, data, phy,     0x11000,  0x1000,
factory,  app,  factory, 0x20000,  0x100000,
ota_0,    app,  ota_0,   0x120000, 0x180000,
ota_1,    app,  ota_1,   0x2a0000, 0x180000,
```

### Grundregeln
- Die `otadata`-Partition ist zwingend erforderlich — ohne sie kann der Bootloader den aktiven OTA-Slot nicht nachverfolgen.
- `ota_0` und `ota_1` müssen gleich groß sein.
- Jeden OTA-Slot anhand der tatsächlichen Binärgröße aus `idf.py size` dimensionieren, mit Sicherheitsmarge (≥20 %).
- Den Subtyp `factory` niemals für einen OTA-Slot verwenden; er ist für das Recovery-/Golden-Image reserviert.
- `CONFIG_PARTITION_TABLE_CUSTOM=y` setzen und `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` auf die eigene CSV-Datei zeigen lassen.

## OTA-Update-API-Ablauf

### Grundlegende In-App-OTA-Sequenz
```c
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"

esp_err_t perform_ota(const uint8_t *data, size_t total_size)
{
    const esp_partition_t *update_partition =
        esp_ota_get_next_update_partition(NULL);  // picks the inactive slot
    if (!update_partition) {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_ERR_NOT_FOUND;
    }

    esp_ota_handle_t handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }

    // Write data in chunks as received (e.g. from HTTP stream)
    err = esp_ota_write(handle, data, total_size);
    if (err != ESP_OK) {
        esp_ota_abort(handle);
        return err;
    }

    err = esp_ota_end(handle);  // validates the image
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s (image may be corrupt)", esp_err_to_name(err));
        return err;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "OTA complete. Rebooting into new firmware.");
    esp_restart();
    return ESP_OK;  // unreachable
}
```

### HTTPS-OTA (empfohlen für Netzwerk-Updates)
```c
#include "esp_https_ota.h"

void ota_task(void *arg)
{
    esp_http_client_config_t http_cfg = {
        .url = CONFIG_OTA_FIRMWARE_URL,
        .cert_pem = server_cert_pem_start,  // embed via component CMakeLists EMBED_TXTFILES
        .timeout_ms = 5000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
        .http_client_init_cb = NULL,
        .bulk_flash_erase = false,           // set true only for very large images
        .partial_http_download = false,
    };

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(TAG, "HTTPS OTA failed: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}
```

### Streaming-HTTPS-OTA (Chunk-für-Chunk, für Fortschrittsanzeige)
```c
esp_https_ota_handle_t ota_handle;
esp_err_t err = esp_https_ota_begin(&ota_cfg, &ota_handle);

int image_len = esp_https_ota_get_image_size(ota_handle);
while (true) {
    err = esp_https_ota_perform(ota_handle);
    if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;
    int written = esp_https_ota_get_image_len_read(ota_handle);
    ESP_LOGI(TAG, "OTA progress: %d / %d bytes", written, image_len);
}

if (esp_https_ota_is_complete_data_received(ota_handle)) {
    err = esp_https_ota_finish(ota_handle);
    if (err == ESP_OK) esp_restart();
} else {
    esp_https_ota_abort(ota_handle);
}
```

## Rollback und Anti-Rollback

### App-Rollback aktivieren
```
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

Mit aktiviertem Rollback bootet das neue Image nach `esp_ota_set_boot_partition()` + Neustart im
Zustand `ESP_OTA_IMG_PENDING_VERIFY`. Die App **muss** aufrufen:
```c
esp_ota_mark_app_valid_cancel_rollback();
```
bevor irgendein Watchdog oder Neustart auslöst. Tut sie das nicht, fällt der Bootloader beim
nächsten Boot auf den vorherigen Slot zurück.

### Rollback-Entscheidungsmuster
```c
void app_main(void)
{
    // Early: check if we're running a newly OTA'd image
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // Run diagnostics before committing
            if (self_test_passed()) {
                ESP_LOGI(TAG, "Self-test passed. Committing OTA image.");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Self-test FAILED. Rolling back.");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
    // Continue normal app startup...
}
```

### Anti-Rollback (Sicherheits-Zähler)
Verhindert das Downgrade auf eine verwundbare Firmware-Version.
```
CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=y
CONFIG_BOOTLOADER_APP_SEC_VER=1         # increment with each security-relevant release
CONFIG_BOOTLOADER_EFUSE_SECURE_VERSION_SCHEME=COUNTER  # or DIGEST
```
- `CONFIG_BOOTLOADER_APP_SEC_VER` nur bei sicherheitsrelevanten Fixes erhöhen — kann nicht verringert werden.
- Der Bootloader liest die Sicherheitsversion aus dem eFuse und verweigert das Booten jedes Images mit einer niedrigeren Version.

## Wichtige sdkconfig-Optionen

| Option | Zweck |
|---|---|
| `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` | Automatischen Rollback aktivieren, falls die App sich nicht selbst validiert |
| `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK` | Firmware mit niedrigerem Sicherheitsversions-Zähler ablehnen |
| `CONFIG_BOOTLOADER_APP_SEC_VER` | In diesem Build fest hinterlegter Wert des Sicherheitsversions-Zählers |
| `CONFIG_OTA_ALLOW_HTTP` | Reines HTTP für OTA erlauben (nur für Entwicklung — niemals in Produktion) |
| `CONFIG_ESP_HTTPS_OTA_DECRYPT_CB` | Eigener Entschlüsselungs-Callback für verschlüsselte OTA-Images |
| `CONFIG_PARTITION_TABLE_CUSTOM` | Projektspezifische Partitions-CSV verwenden |

## Diagnose-Befehle für den OTA-Zustand

```c
// Log running, boot, and next-update partitions
const esp_partition_t *running  = esp_ota_get_running_partition();
const esp_partition_t *boot     = esp_ota_get_boot_partition();
const esp_partition_t *next     = esp_ota_get_next_update_partition(NULL);

ESP_LOGI(TAG, "Running: %s @ 0x%08" PRIx32, running->label, running->address);
ESP_LOGI(TAG, "Boot:    %s @ 0x%08" PRIx32, boot->label,    boot->address);
ESP_LOGI(TAG, "Update:  %s @ 0x%08" PRIx32, next->label,    next->address);

// Log OTA state of running partition
esp_ota_img_states_t state;
if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
    ESP_LOGI(TAG, "OTA state: %d (%s)", state,
        state == ESP_OTA_IMG_NEW            ? "NEW"            :
        state == ESP_OTA_IMG_PENDING_VERIFY ? "PENDING_VERIFY" :
        state == ESP_OTA_IMG_VALID          ? "VALID"          :
        state == ESP_OTA_IMG_INVALID        ? "INVALID"        :
        state == ESP_OTA_IMG_ABORTED        ? "ABORTED"        : "UNDEFINED");
}
```

## Häufige Fehlerbilder

| Symptom | Wahrscheinliche Ursache |
|---|---|
| Bootloader bootet immer `ota_0` | `otadata`-Partition wurde gelöscht oder nie beschrieben; `idf.py erase-flash` ausführen und neu flashen |
| Rollback bei jedem Boot | App ruft nie `esp_ota_mark_app_valid_cancel_rollback()` auf |
| `esp_ota_end` liefert `ESP_ERR_OTA_VALIDATE_FAILED` | Image-Hash-Prüfung fehlgeschlagen — Datenkorruption bei der Übertragung |
| HTTPS-OTA scheitert mit `ESP_ERR_HTTP_CONNECT` | Server-Zertifikat nicht eingebettet oder `cert_pem`-Pointer falsch |
| OTA-Slot zu klein | Binärdatei ist über die Slot-Größe hinausgewachsen — mit `idf.py size` neu berechnen und CSV verbreitern |
| `esp_ota_begin` scheitert mit `ESP_ERR_INVALID_SIZE` | Parameter `image_size` zu klein; `OTA_WITH_SEQUENTIAL_WRITES` verwenden |

## OTA und Secure Boot

- Secure Boot prüft die Signatur des Images beim Booten; OTA-Images müssen mit demselben Schlüssel signiert sein.
- `idf.py secure-target sign-data` oder den `--sign-key`-Pfad des Build-Systems nutzen, um die Binärdatei vor dem Ausliefern zu signieren.
- Bei aktivierter Flash-Verschlüsselung wird die OTA-Partition beim Schreiben automatisch verschlüsselt — die Klartext-Binär-URL ist korrekt; der ESP32 verschlüsselt an Ort und Stelle.
- `CONFIG_SECURE_BOOT_ALLOW_ROM_BASIC` oder `CONFIG_SECURE_BOOT_ALLOW_JTAG` nicht in der Entwicklung deaktivieren und dann vergessen, die Einschränkungen für Produktions-Builds wieder zu aktivieren.
