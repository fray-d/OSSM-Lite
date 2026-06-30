#include <queue>

#include "Arduino.h"
#include "NimBLECharacteristic.h"
#include "NimBLEService.h"
#include "NimBLEUUID.h"

class FTSCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();

        // Expected format: [position, timeHigh, timeLow]
        // position: uint8 (0-180), convert to 100
        // time: uint16 big-endian (MSB first)
        if (value.length() >= 3) {
            uint8_t position = static_cast<uint8_t>(value[0] / 1.8);
            uint16_t time = (static_cast<uint8_t>(value[1]) << 8) |
                            static_cast<uint8_t>(value[2]);

            ESP_LOGI("NIMBLE", "FTS Command - Position: %d, Time: %d ms", position, time);
            targetQueue.push({position, time, std::chrono::steady_clock::now(), 0});

        } else {
            ESP_LOGW("NIMBLE", "FTS write - Invalid data length: %d bytes",
                     value.length());
        }
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String ftsValue = String(value.c_str());
        pCharacteristic->setValue(ftsValue);
        // Print everything that comes to this
        ESP_LOGD("NIMBLE", "FTS read: %s", ftsValue.c_str());
    }

    /** Peer subscribed to notifications/indications */
    void onSubscribe(NimBLECharacteristic* pCharacteristic,
                     NimBLEConnInfo& connInfo, uint16_t subValue) override {
        std::string str = "Client ID: ";
        str += connInfo.getConnHandle();
        str += " Address: ";
        str += connInfo.getAddress().toString();
        if (subValue == 0) {
            str += " Unsubscribed to ";
        } else if (subValue == 1) {
            str += " Subscribed to notifications for ";
        } else if (subValue == 2) {
            str += " Subscribed to indications for ";
        } else if (subValue == 3) {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic->getUUID());
        ESP_LOGV("NIMBLE", "%s", str.c_str());
    }

} ftsCallbacks;

inline NimBLECharacteristic* initFleshyCharacteristic(NimBLEService* pService) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(NimBLEUUID(FLESHY_COMMAND_UUID),
                    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::WRITE_NR);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Fleshy thrust sync commands");
    pChar->setCallbacks(&ftsCallbacks);
    return pChar;
}

