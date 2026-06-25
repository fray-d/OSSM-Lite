#ifndef OSSM_PATTERNS_HPP
#define OSSM_PATTERNS_HPP

#include "ArduinoJson.h"
#include "NimBLEService.h"
#include "Strings.h"
#include "esp_log.h"

inline NimBLECharacteristic* initPatternsCharacteristic(NimBLEService* pService,
                                                 NimBLEUUID uuid) {
    // Patterns characteristic (read-only list of all patterns)
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ);

    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("List of available patterns");

    // Use ArduinoJson to construct the patterns JSON
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < sizeof(ui::strings::strokeEngineNames) /
                            sizeof(ui::strings::strokeEngineNames[0]);
         i++) {
        JsonObject pattern = arr.add();
        pattern["name"] = ui::strings::strokeEngineNames[i];
        pattern["idx"] = i;
    }

    String jsonString;
    serializeJson(arr, jsonString);
    pChar->setValue(jsonString.c_str());
    return pChar;
}

class PatternDataCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String patternValue = String(value.c_str());

        // Parse the integer input
        int patternIndex = patternValue.toInt();

        // Get the size of the StrokeEngineDescriptions array
        int descriptionsCount =
            sizeof(ui::strings::strokeEngineDescriptions) /
            sizeof(ui::strings::strokeEngineDescriptions[0]);

        // Use modulo to ensure valid index
        int validIndex = patternIndex % descriptionsCount;
        if (validIndex < 0) {
            validIndex += descriptionsCount;  // Handle negative numbers
        }

        // Get the pattern description from PROGMEM
        const char* description =
            ui::strings::strokeEngineDescriptions[validIndex];

        // Set the characteristic value to the description
        pCharacteristic->setValue(description);

        ESP_LOGD("Pattern Callback",
                 "Pattern description requested: index=%d, validIndex=%d, "
                 "description=%s",
                 patternIndex, validIndex, description);
    }

    void onRead(NimBLECharacteristic* pCharacteristic,
                NimBLEConnInfo& connInfo) override {
        // Return the current value (set by onWrite)
        std::string value = pCharacteristic->getValue();
        ESP_LOGD("Pattern Callback", "Pattern description read: %s", value.c_str());
    }
} inline patternDataCallbacks;

inline NimBLECharacteristic* initPatternDataCharacteristic(NimBLEService* pService,
                                                    NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);

    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Pattern description lookup");

    pChar->setCallbacks(&patternDataCallbacks);
    return pChar;
}

#endif  // OSSM_PATTERNS_HPP
