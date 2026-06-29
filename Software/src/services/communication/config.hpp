#ifndef OSSM_CONFIG_HPP
#define OSSM_CONFIG_HPP

#include <NimBLECharacteristic.h>
#include <NimBLEService.h>
#include <services/board.h>

#include "Arduino.h"
#include "services/led.h"
#include "services/UserConfig.h"

void setBoolValue(bool value, NimBLECharacteristic* pCharacteristic) {
    // Use PROGMEM strings for read values
    static const char true_str[] PROGMEM = "true";
    static const char false_str[] PROGMEM = "false";

    String output = value ? String(FPSTR(true_str)) : String(FPSTR(false_str));
    pCharacteristic->setValue(output);
}

/** Handler class for speed knob config characteristic */
class SpeedKnobConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String configValue = String(value.c_str());
        configValue.toLowerCase();

        ESP_LOGD("NIMBLE", "Speed knob config write: %s", configValue.c_str());

        // Store config strings in PROGMEM
        static const char true_str[] PROGMEM = "true";
        static const char false_str[] PROGMEM = "false";
        static const char error_invalid[] PROGMEM = "error:invalid_value";

        if (configValue == "true" || configValue == "1" || configValue == "t") {
            USE_SPEED_KNOB_AS_LIMIT = true;
            pCharacteristic->setValue(String(FPSTR(true_str)));
        } else if (configValue == "false" || configValue == "0" ||
                   configValue == "f") {
            USE_SPEED_KNOB_AS_LIMIT = false;
            pCharacteristic->setValue(String(FPSTR(false_str)));
        } else {
            ESP_LOGW("NIMBLE", "Invalid speed knob config value: %s",
                     configValue.c_str());
            pCharacteristic->setValue(String(FPSTR(error_invalid)));
        }
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic,
                NimBLEConnInfo& connInfo) override {
        setBoolValue(USE_SPEED_KNOB_AS_LIMIT, pCharacteristic);
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGV(
            "NIMBLE",
            "Speed knob config notification/indication return code: %d, %s",
            code, NimBLEUtils::returnCodeToString(code));
    }
} inline speedKnobConfigCallbacks;

inline NimBLECharacteristic* initSpeedKnobConfigCharacteristic(NimBLEService* pService,
                                                        NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
        
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Use wired controller knob as speed limit.");

    pChar->setCallbacks(&speedKnobConfigCallbacks);

    setBoolValue(USE_SPEED_KNOB_AS_LIMIT, pChar);

    return pChar;
}

/** Handler class for latency compensation config characteristic */
class LatencyCompensationConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String configValue = String(value.c_str());
        configValue.toLowerCase();

        ESP_LOGD("NIMBLE", "Latency compensation config write: %s",
                 configValue.c_str());

        // Store config strings in PROGMEM
        static const char true_str[] PROGMEM = "true";
        static const char false_str[] PROGMEM = "false";
        static const char error_invalid[] PROGMEM = "error:invalid_value";

        if (configValue == "true" || configValue == "1" || configValue == "t") {
            USE_LATENCY_COMPENSATION = true;
            pCharacteristic->setValue(String(FPSTR(true_str)));
        } else if (configValue == "false" || configValue == "0" ||
                   configValue == "f") {
            USE_LATENCY_COMPENSATION = false;
            pCharacteristic->setValue(String(FPSTR(false_str)));
        } else {
            ESP_LOGW("NIMBLE", "Invalid latency compensation config value: %s",
                     configValue.c_str());
            pCharacteristic->setValue(String(FPSTR(error_invalid)));
        }
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic,
                NimBLEConnInfo& connInfo) override {
        setBoolValue(USE_LATENCY_COMPENSATION, pCharacteristic);
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGV("NIMBLE",
                 "Latency compensation config notification/indication return "
                 "code: %d, %s",
                 code, NimBLEUtils::returnCodeToString(code));
    }
} inline latencyCompensationConfigCallbacks;

inline NimBLECharacteristic* initLatencyCompensationConfigCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Enable latency compensation for streaming mode.");

    pChar->setCallbacks(&latencyCompensationConfigCallbacks);

    setBoolValue(USE_LATENCY_COMPENSATION, pChar);

    return pChar;
}

class RenameConfigCallbacks : public NimBLECharacteristicCallbacks {
    int lastPresetCommand = millis();

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        int currentTime = millis();
        if (currentTime - lastPresetCommand > 1000) {
            String value = pCharacteristic->getValue();
            value = value.substring(0, 8);
            UserConfig::setDeviceName(value);
        }
        lastPresetCommand = currentTime;
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic,
                NimBLEConnInfo& connInfo) override {
        std::string name = UserConfig::getDeviceName();
        pCharacteristic->setValue(name);
        ESP_LOGD("NIMBLE_RENAME", "Name read: %s", name.c_str());
    }
} inline renameConfigCallbacks;

inline NimBLECharacteristic* initRenameConfigCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ);

    pChar->setCallbacks(&renameConfigCallbacks);
    pChar->setValue(UserConfig::getDeviceName());
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Name of the device. Changing will cause reboot.");

    return pChar;
}

class DirectionConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String configValue = String(value.c_str());
        configValue.toLowerCase();
        if (configValue == "true" || configValue == "1" || configValue == "t") {
            UserConfig::setDirection(true);
        } else if (configValue == "false" || configValue == "0" || configValue == "f") {
            UserConfig::setDirection(false);
        } else {
            ESP_LOGW("NIMBLE", "Invalid direction config value: %s",configValue.c_str());
            pCharacteristic->setValue("error:invalid_value");
        }
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        setBoolValue(UserConfig::getDirection(), pCharacteristic);
    }
} inline directionConfigCallbacks;

inline NimBLECharacteristic* initDirectionConfigCharacteristic(
    NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ);

    pChar->setCallbacks(&directionConfigCallbacks);
    setBoolValue(UserConfig::getDirection(), pChar);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Reverse rail direction. Changing will cause reboot.");

    return pChar;
}

class HomingTypeConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        UserConfig::HomingType type = static_cast<UserConfig::HomingType>(std::stoi(value)); 
        UserConfig::setHomingType(type);
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(UserConfig::getHomingType()));
    }
} inline homingTypeConfigCallbacks;

inline NimBLECharacteristic* initHomingTypeConfigCharacteristic(
    NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ);

    pChar->setCallbacks(&homingTypeConfigCallbacks);
    pChar->setValue(UserConfig::getHomingType());
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Homing Type: 0=None, 1=Default, 2=Single Sided, 3=Double Tap");
    return pChar;
}

class RailLengthConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        float value = std::stof(pCharacteristic->getValue());
        UserConfig::setRailLength(value);
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(String(UserConfig::getRailLength()));
    }
} inline railLengthConfigCallbacks;

inline NimBLECharacteristic* initRailLengthConfigCharacteristic(
    NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ);

    pChar->setCallbacks(&railLengthConfigCallbacks);
    pChar->setValue(UserConfig::getRailLength());
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Rail length. Used for single sided and disabled homing.");
    return pChar;
}

class ReHomeConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        String configValue = String(value.c_str());
        configValue.toLowerCase();
        if (configValue == "true" || configValue == "1" || configValue == "t") {
            UserConfig::setReHome(true);
        } else if (configValue == "false" || configValue == "0" || configValue == "f") {
            UserConfig::setReHome(false);
        } else {
            ESP_LOGW("NIMBLE", "Invalid home between modes config value: %s",configValue.c_str());
            pCharacteristic->setValue("error:invalid_value");
        }
        pulseForCommunication();
    }

    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        setBoolValue(UserConfig::getReHome(), pCharacteristic);
    }
} inline ReHomeConfigCallbacks;

inline NimBLECharacteristic* initReHomeConfigCharacteristic(
    NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(
        uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ);

    pChar->setCallbacks(&ReHomeConfigCallbacks);
    setBoolValue(UserConfig::getReHome(), pChar);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Rehome the device between modes.");

    return pChar;
}

#endif  // OSSM_CONFIG_HPP
