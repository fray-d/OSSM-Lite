#include "nimble.h"

#include <services/tasks.h>

#include <queue>

#include "command.hpp"
#include "config.hpp"
#include "gpio.hpp"
#include "ossm/OSSM.h"
#include "ossm/advanced_penetration/advanced_penetration.h"
#include "patterns.hpp"
#include "services/led.h"
#include "services/UserConfig.h"
#include "state.hpp"
#include "wifi.hpp"
#include "constants/Version.h"

// Define the global variables
NimBLEServer* pServer = nullptr;

NimBLECharacteristic* pStateCharacteristic = nullptr;
NimBLECharacteristic* pSpeedKnobConfigCharacteristic = nullptr;
NimBLECharacteristic* pLatencyCompensationConfigCharacteristic = nullptr;
NimBLECharacteristic* pCommandCharacteristic = nullptr;

static long lostConnectionTime = 0;
static int speedOnLostConnection = 0;
static const unsigned long RAMP_DURATION_MS =
    2000;  // Duration for speed ramp to zero

double easeInOutSine(double t) {
    return 0.5 * (1 + sin(3.1415926 * (t - 0.5)));
}

/** Handler class for server actions */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI("NIMBLE", "Client connected: %s",
                 connInfo.getAddress().toString().c_str());
        ESP_LOGI("NIMBLE", "Connection count: %d",
                 pServer->getConnectedCount());

        lostConnectionTime = 0;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo,
                      int reason) override {
        ESP_LOGI("NIMBLE", "Client disconnected: %s, reason: %d",
                 connInfo.getAddress().toString().c_str(), reason);
        ESP_LOGI("NIMBLE", "Connection count: %d",
                 pServer->getConnectedCount());

        stateMachine->process_event(ReturnToMenu{});

        // Capture current speed when connection is lost
        speedOnLostConnection = settings.speed;
        ESP_LOGI("NIMBLE", "Speed on disconnect: %d", speedOnLostConnection);

        lostConnectionTime = millis();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        ESP_LOGD("NIMBLE", "MTU changed to: %d for connection: %s", MTU,
                 connInfo.getAddress().toString().c_str());
    }
} serverCallbacks;

#ifdef PRETEND_TO_BE_FLESHY_THRUST_SYNC
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
#endif

void nimbleLoop(void* pvParameters) {
    NimBLEServer* pServer = (NimBLEServer*)pvParameters;
    /** Loop here and send notifications to connected peers */

    String lastState = "";
    int lastConnCount = 0;
    int lastMessageTime = 0;
    while (true) {
        // Check if we should be advertising (no connections)
        if (pServer->getConnectedCount() == 0) {
            // If not advertising and no connections, restart advertising
            if (stateMachine->is("menu.idle"_s) || stateMachine->is("error.idle"_s)) {
                if (!pServer->getAdvertising()->isAdvertising()) {
                    pServer->startAdvertising();
                    ESP_LOGI("NIMBLE",
                            "No connections and not advertising, restarting "
                            "advertising");
                }
            } else {
                pServer->stopAdvertising();
            }

            if (lostConnectionTime > 0) {
                // Skip ramp-down if speed was already zero when connection was
                // lost
                if (speedOnLostConnection <= 0) {
                    lostConnectionTime = 0;
                    continue;
                }

                unsigned long elapsed = millis() - lostConnectionTime;

                // Wait 1 second before starting easing
                if (elapsed < 1000) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue;
                }

                if (elapsed > 1000 + RAMP_DURATION_MS) {
                    ESP_LOGI(
                        "NIMBLE",
                        "Speed ramp duration exceeded, setting speed to 0");
                    lostConnectionTime = 0;
                    speedOnLostConnection = 0;
                    ossm->ble_click("set:speed:0");
                    continue;
                }

                // Calculate easing factor (0 to 1) over ramp duration
                double progress = constrain(
                    (elapsed - 1000) / (double)RAMP_DURATION_MS, 0.0, 1.0);
                double t = easeInOutSine(progress);

                // Ramp from current speed to zero
                int targetSpeed = (int)(speedOnLostConnection * (1.0 - t));
                ESP_LOGI("NIMBLE", "Target speed: %d (from %d, progress: %.2f)",
                         targetSpeed, speedOnLostConnection, progress);

                ossm->ble_click("set:speed:" + String(targetSpeed));

                // Stop processing when easing is complete
                if (t >= 1) {
                    lostConnectionTime = 0;
                    continue;
                }

                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        int currentConnCount = pServer->getConnectedCount();

        // Clear last state when connection count changes
        if (currentConnCount != lastConnCount) {
            lastState = "";
            lastConnCount = currentConnCount;
        }

        NimBLEService* pSvc = pServer->getServiceByUUID(SERVICE_UUID);
        if (!pSvc) {
            lastState = "";
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        NimBLECharacteristic* pChr = pSvc->getCharacteristic(CHARACTERISTIC_STATE_UUID);
        if (!pChr) {
            lastState = "";
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // mannage message queue
        while (!messageQueue.empty()) {
            String cmd = messageQueue.front();
            messageQueue.pop();
            ossm->ble_click(cmd);
            pChr->setValue("ok:" + cmd);

            // Trigger LED communication pulse for command processing
            pulseForCommunication();

            vTaskDelay(1);
        }

        int currentTime = millis();
        String fingerprint = ossm->getStateFingerprint();
        bool stateChanged = fingerprint != lastState;
        bool timeElapsed = (currentTime - lastMessageTime) > 1000;

        if (!stateChanged && !timeElapsed) {
            vTaskDelay(100);
            continue;
        }
        lastMessageTime = currentTime;
        lastState = fingerprint;

        String currentState = ossm->getCurrentState();
        if (stateChanged) {
            ESP_LOGD("NIMBLE", "State changed to: %s", currentState.c_str());
            pChr->setValue(currentState);
            pChr->notify();
            // Trigger LED communication pulse for state update
            pulseForCommunication();
        }

        lastState = fingerprint;
        vTaskDelay(1);
    }
}

void initNimble() {
    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init(UserConfig::getDeviceName());

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    // Create Service
    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    pCommandCharacteristic = initCommandCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_UUID));

    pSpeedKnobConfigCharacteristic = initSpeedKnobConfigCharacteristic(
                        pService, NimBLEUUID(CHARACTERISTIC_SPEED_KNOB_CONFIG_UUID));

    pLatencyCompensationConfigCharacteristic = initLatencyCompensationConfigCharacteristic(
                        pService, NimBLEUUID(CHARACTERISTIC_LATENCY_COMPENSATION_CONFIG_UUID));

    initWiFiConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_WIFI_CONFIG_UUID));

    initRenameConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_RENAME_CONFIG_UUID));

    initDirectionConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_DIRECTION_CONFIG_UUID));

    pStateCharacteristic = initStateCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_STATE_UUID));

    initHomingTypeConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_HOMING_TYPE_CONFIG_UUID));
    
    initRailLengthConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_RAIL_LENGTH_CONFIG_UUID));

    initReHomeConfigCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_HOME_BETWEEN_MODES_CONFIG_UUID));

    pStateCharacteristic = initStateCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_STATE_UUID));

    initPatternsCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_PATTERNS_UUID));
    initPatternDataCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_GET_PATTERN_DATA_UUID));

    // GPIO write/read characteristic
    initGPIOCharacteristic(pService, NimBLEUUID(CHARACTERISTIC_GPIO_UUID));

    // Add Device Information Service
    NimBLEService* pDeviceInfoService = pServer->createService(DEVICE_INFO_SERVICE_UUID);

    // Add Manufacturer Name characteristic
    NimBLECharacteristic* pManufacturerName = pDeviceInfoService->createCharacteristic(MANUFACTURER_NAME_UUID, NIMBLE_PROPERTY::READ);
    pManufacturerName->setValue(ui::strings::kinkyMakers);

    // Add Model 
    NimBLECharacteristic* pModel = pDeviceInfoService->createCharacteristic(MODEL_UUID, NIMBLE_PROPERTY::READ);
    pModel->setValue(ui::strings::deviceName);


    // Add Firmware Version
    NimBLECharacteristic* pVersion = pDeviceInfoService->createCharacteristic(FIRMWARE_VERSION_UUID, NIMBLE_PROPERTY::READ);
    pVersion->setValue(VERSION);

#ifdef PRETEND_TO_BE_FLESHY_THRUST_SYNC
    // if this is true, then we'll start a service for the FTS
    NimBLEService* pFTS = pServer->createService(
        NimBLEUUID("0000ffe0-0000-1000-8000-00805f9b34fb"));
    NimBLECharacteristic* pChar = pFTS->createCharacteristic(
        NimBLEUUID("0000ffe1-0000-1000-8000-00805f9b34fb"),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE |
            NIMBLE_PROPERTY::WRITE_NR);
    NimBLEDescriptor* pDesc = pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ);
    pDesc->setValue("Fleshy thrust sync commands");
    pChar->setCallbacks(&ftsCallbacks);
#endif

    NimBLEService* aService = advanced_penetration::initNimble();

    // Update advertising to include new services
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(UserConfig::getDeviceName());
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->addServiceUUID(pDeviceInfoService->getUUID());
    pAdvertising->addServiceUUID(aService->getUUID());
#ifdef PRETEND_TO_BE_FLESHY_THRUST_SYNC
    pAdvertising->addServiceUUID(pFTS->getUUID());
#endif
    pAdvertising->enableScanResponse(true);

    pAdvertising->start();

    xTaskCreatePinnedToCore(
        nimbleLoop, "nimbleLoop", 5 * configMINIMAL_STACK_SIZE, pServer,
        configMAX_PRIORITIES - 1, nullptr, Tasks::stepperCore);
}
