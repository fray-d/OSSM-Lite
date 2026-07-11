#include <Arduino.h>
#include <esp_crt_bundle.h>
#include <esp_heap_caps.h>
#include <esp_https_ota.h>
#include "ossm/pages/update.h"
#include "ossm/state/state.h"
#include "services/led.h"
#include "Strings.h"

static void updateTask(void *pvParameters) {
    ESP_LOGW("UPDATE", "Update task started (free heap: %lu, largest block: %lu)",
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));


    pages::drawUpdating();

    String url = String(ui::strings::updateURL) + "firmware.bin";
    ESP_LOGW("UPDATE", "Starting OTA from %s (free heap: %lu, largest block: %lu)",
             url.c_str(), (unsigned long)esp_get_free_heap_size(),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    esp_http_client_config_t httpConfig = {};
    httpConfig.url = url.c_str();
    httpConfig.timeout_ms = 30000;
    httpConfig.buffer_size = 4096;
    httpConfig.buffer_size_tx = 1024;
    httpConfig.crt_bundle_attach = esp_crt_bundle_attach;

    esp_err_t ret = esp_https_ota(&httpConfig);
    if (ret == ESP_OK) {
        ESP_LOGW("UPDATE", "OTA succeeded, restarting...");
        esp_restart();
    }

    ESP_LOGE("UPDATE", "OTA failed: %s", esp_err_to_name(ret));
    stateMachine->process_event(UpdateUnavailable{});
    vTaskDelete(nullptr);
}

void ossmStartUpdate() {
    xTaskCreatePinnedToCore(updateTask, "updateTask",
                            20 * configMINIMAL_STACK_SIZE, nullptr, 1, nullptr,
                            0);
}
