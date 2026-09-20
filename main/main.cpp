#include "app_shell.h"

#include "esp_err.h"
#include "joint_tracker_service.h"
#include "nvs_flash.h"

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 420-Track laedt seinen Stand aus NVS und holt einen faelligen Tageswechsel
    // gleich nach, bevor die Startseite zum ersten Mal gezeichnet wird.
    joint_tracker_service::Load();

    app_shell::Run();
}
