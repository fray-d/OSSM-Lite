#ifndef OSSM_COMMUNICATION_COMMAND_HPP
#define OSSM_COMMUNICATION_COMMAND_HPP

// #include <queue>
#include <regex>

#include "ossm/state/ble.h"
#include "ossm/state/menu.h"
#include "ossm/state/settings.h"
#include "ossm/state/state.h"
#include "queue.h"
#include "services/encoder.h"
#include "services/led.h"

static const std::regex commandRegex(
    R"(go:(strokeEngine|streaming|menu)|set:(speed|stroke|depth|sensation|buffer|pattern):\d+|set:wifi:[^|]+\|.+|stream:\d+:\d+)");

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string cmd = pCharacteristic->getValue();
        if (!std::regex_match(cmd, commandRegex)) {
            ESP_LOGD("NIMBLE_COMMAND", "Invalid command: %s", cmd.c_str());
            pCharacteristic->setValue("fail:" + String(cmd.c_str()));
            return;
        }
        messageQueue.push(String(cmd.c_str()));
        pulseForCommunication();
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGV("NIMBLE_COMMAND",
                 "Notification/Indication return code: %d, %s", code,
                 NimBLEUtils::returnCodeToString(code));
    }
} inline chrCallbacks;

inline NimBLECharacteristic* initCommandCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Input commands to the device.");
    pChar->setCallbacks(&chrCallbacks);
    return pChar;
}

NimBLECharacteristic* initCommonCharacteristic(NimBLEService* pService, std::string uuid, NimBLECharacteristicCallbacks* callbacks, String description) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue(description);
    pChar->setCallbacks(callbacks);
    return pChar;
}

class SpeedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        bleState.lastSpeedCommandWasFromBLE = true;
        settings.speedBLE = constrain(value, 0.0, 100.0);
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.speed));
    }
} inline speedCallbacks;

class MaxDepthCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        settings.maxPosition = constrain(value, 0.0, 100.0);
        settings.playControl = ui::PlayControls::MAX_POSITION;
        encoder.setEncoderValue(settings.maxPosition);
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.maxPosition));
    }
} inline maxDepthCallbacks;

class MinDepthCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        settings.minPosition = constrain(value, 0.0, 100.0);
        settings.playControl = ui::PlayControls::MIN_POSITION;
        encoder.setEncoderValue(settings.minPosition);
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.minPosition));
    }
} inline minDepthCallbacks;

class OffsetCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.buffer));
    }
} inline offsetCallbacks;

NimBLECharacteristic* initOffsetCharacteristic(NimBLEService* pService, std::string uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Provides offset for automatic sync");
    pChar->setCallbacks(&offsetCallbacks);
    return pChar;
}

class StreamCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        String cmd = pCharacteristic->getValue();
        uint8_t split = cmd.indexOf(':');
        float pos = constrain(cmd.substring(0,split).toFloat(), 0.0, 100.0);
        uint16_t time = cmd.substring(split+1).toInt();
        ESP_LOGI("STREAM", "Move to: %f in %d", pos, time);
        targetQueue.push({pos,time,std::chrono::steady_clock::now(), 0});
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        if (!(stateMachine->is("streaming"_s) || stateMachine->is("streaming.idle"_s))) {
            stateMachine->process_event(LongPress{});
            menuState.currentOption = Menu::Streaming;
            stateMachine->process_event(ButtonPress{});
        }
        pCharacteristic->setValue(String("Ready"));
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGV("NIMBLE_COMMAND", "Notification/Indication return code: %d, %s", code, NimBLEUtils::returnCodeToString(code));
    }
} inline streamCallbacks;

#endif  // OSSM_COMMUNICATION_COMMAND_HPP
