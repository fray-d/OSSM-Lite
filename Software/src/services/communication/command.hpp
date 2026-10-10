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
    R"(go:(strokeEngine|streaming|update|menu)|set:(max|min|speed|stroke|depth|sensation|buffer|pattern):(\d|\.)+|set:wifi:[^|]+\|.+|stream:(\d|\.)+:\d+)");

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
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
    pChar->setCallbacks(&chrCallbacks);
    return pChar;
}

NimBLECharacteristic* initCharacteristic(NimBLEService* pService, std::string uuid, NimBLECharacteristicCallbacks* callbacks, String description) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pChar->setCallbacks(callbacks);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue(description);
    return pChar;
}

NimBLECharacteristic* initPatternCommandCharacteristic(NimBLEService* pService, std::string uuid, NimBLECharacteristicCallbacks* callbacks) {
    NimBLECharacteristic* pChar = initCharacteristic(pService, uuid, callbacks, "Pattern");
    NimBLE2904* p2904 = pChar->create2904();
    p2904->setFormat(p2904->FORMAT_UINT16);
    uint16_t minValue = 0;
    uint16_t maxValue = sizeof(ui::strings::strokeEngineNames) / sizeof(ui::strings::strokeEngineNames[0]) - 1;
    uint8_t rangeBuffer[4];
    memcpy(&rangeBuffer[0], &minValue, sizeof(minValue));
    memcpy(&rangeBuffer[2], &maxValue, sizeof(maxValue));
    NimBLEDescriptor* pRange = pChar->createDescriptor("2906", NIMBLE_PROPERTY::READ, sizeof(rangeBuffer));
    pRange->setValue(rangeBuffer, sizeof(rangeBuffer));
    return pChar;
}

NimBLECharacteristic* initStreamCharacteristic(NimBLEService* pService, std::string uuid, NimBLECharacteristicCallbacks* callbacks, String description, uint16_t ordinal) {
    NimBLECharacteristic* pChar = initCharacteristic(pService, uuid, callbacks, description);
    NimBLE2904* p2904 = pChar->create2904();
    p2904->setFormat(p2904->FORMAT_OPAQUE);
    p2904->setDescription(ordinal);
    float minValue = 0.0;
    float maxValue = 100.0;
    uint8_t rangeBuffer[8];
    memcpy(&rangeBuffer[0], &minValue, sizeof(minValue));
    memcpy(&rangeBuffer[4], &maxValue, sizeof(maxValue));
    NimBLEDescriptor* pRange = pChar->createDescriptor("2906", NIMBLE_PROPERTY::READ, sizeof(rangeBuffer));
    pRange->setValue(rangeBuffer, sizeof(rangeBuffer));
    return pChar;
}

//0x1B struct opaque structure

NimBLECharacteristic* initFloatCharacteristic(NimBLEService* pService, std::string uuid, NimBLECharacteristicCallbacks* callbacks, String description, uint16_t ordinal) {
    NimBLECharacteristic* pChar = initCharacteristic(pService, uuid, callbacks, description);
    NimBLE2904* p2904 = pChar->create2904();
    p2904->setFormat(p2904->FORMAT_SFLOAT32);
    p2904->setDescription(ordinal);
    float minValue = 0.0;
    float maxValue = 100.0;
    uint8_t rangeBuffer[8];
    memcpy(&rangeBuffer[0], &minValue, sizeof(minValue));
    memcpy(&rangeBuffer[4], &maxValue, sizeof(maxValue));
    NimBLEDescriptor* pRange = pChar->createDescriptor("2906", NIMBLE_PROPERTY::READ, sizeof(rangeBuffer));
    pRange->setValue(rangeBuffer, sizeof(rangeBuffer));
    return pChar;
}

class SpeedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        bleState.lastSpeedCommandWasFromBLE = true;
        settings.speedBLE = constrain(value, 0.0, 100.0);
        pCharacteristic->setValue(String(value));
        pCharacteristic->notify();
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
        if (settings.maxPosition < settings.minPosition) {
            settings.maxPosition = constrain(settings.minPosition + 1.0, 0.0, 100.0);
        }
        settings.playControl = ui::PlayControls::MAX_POSITION;
        encoder.setEncoderValue(settings.maxPosition);
        pCharacteristic->setValue(String(value));
        pCharacteristic->notify();
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
        if (settings.maxPosition < settings.minPosition) {
            settings.minPosition = constrain(settings.maxPosition - 1.0, 0.0, 100.0);
        }
        settings.playControl = ui::PlayControls::MIN_POSITION;
        encoder.setEncoderValue(settings.minPosition);
        pCharacteristic->setValue(String(value));
        pCharacteristic->notify();
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.minPosition));
    }
} inline minDepthCallbacks;

class SensationCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        settings.sensation = constrain(value, 0.0, 100.0);
        settings.playControl = ui::PlayControls::SENSATION;
        encoder.setEncoderValue(settings.sensation);
        pCharacteristic->setValue(String(value));
        pCharacteristic->notify();
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(settings.sensation));
    }
} inline sensationCallbacks;

void startStrokeEngine() {
    if (!(stateMachine->is("strokeEngine"_s ) || stateMachine->is("strokeEngine.idle"_s) || stateMachine->is("strokeEngine.pattern"_s))) {
        stateMachine->process_event(LongPress{});
        menuState.currentOption = Menu::StrokeEngine;
        settings.speedBLE = 0.0;
        stateMachine->process_event(ButtonPress{});
    }
}

class StrokeEnginePatternCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        startStrokeEngine();
        int value = std::stoi(pCharacteristic->getValue());
        settings.pattern = static_cast<StrokePatterns>((int)value % (int)StrokePatterns::Count);
        pCharacteristic->notify();
        pulseForCommunication();
    }
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String((int)settings.pattern));
    }
} inline strokeEnginePatternCallbacks;

void startStreaming() {
    if (!(stateMachine->is("streaming"_s) || stateMachine->is("streaming.idle"_s))) {
        stateMachine->process_event(LongPress{});
        menuState.currentOption = Menu::Streaming;
        settings.speedBLE = 0.0;
        stateMachine->process_event(ButtonPress{});
    }
}

class StreamCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        String cmd = pCharacteristic->getValue();
        uint8_t split = cmd.indexOf(':');
        float pos = constrain(cmd.substring(0,split).toFloat(), 0.0, 100.0);
        uint16_t time = cmd.substring(split+1).toInt();
        ESP_LOGI("STREAM", "Move to: %f in %d", pos, time);
        startStreaming();
        targetQueue.push({pos,time,std::chrono::steady_clock::now(), 0});
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String("POS:MS"));
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGV("NIMBLE_COMMAND", "Notification/Indication return code: %d, %s", code, NimBLEUtils::returnCodeToString(code));
    }
} inline streamCallbacks;

#endif  // OSSM_COMMUNICATION_COMMAND_HPP