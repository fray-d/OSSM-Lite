#include "wm.h"

#include <ArduinoJson.h>
#include "UserConfig.h"
#include "WiFi.h"
#include "esp_log.h"
#include "esp_wifi.h"

WiFiManager wm;

void initWM() {
    WiFi.useStaticBuffers(true);
    WiFi.setAutoReconnect(false);
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

    wm.setSaveConfigCallback([]() {
        ESP_LOGI("WM", "WiFi credentials saved to NVS");
    });

    WiFi.begin();
    ESP_LOGI("WM", "WiFi initialization complete, status: %d", WiFi.status());
}

bool connectWiFi(const String& ssid, const String& password) {
    if (ssid.length() == 0) {
        ESP_LOGW("WM", "No SSID found");
        return false;
    }

    ESP_LOGI("WM", "Attempting to connect to WiFi: %s", ssid.c_str());

    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
    }

    // Begin connection
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection (timeout after 10 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        ESP_LOGD("WM", "Connecting... attempt %d/20", attempts);
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI("WM", "Connected to WiFi. IP: %s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        ESP_LOGW("WM", "Failed to connect to WiFi. Status: %d", WiFi.status());
        return false;
    }
}

String getWiFiStatus() {
    JsonDocument doc;
    
    bool connected = (WiFi.status() == WL_CONNECTED);
    doc["connected"] = connected;

    if (connected) {
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
    }

    String output;
    serializeJson(doc, output);
    return output;
}
