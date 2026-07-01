#include "nimble.h"

#include <services/tasks.h>

#include "constants/Version.h"
#include "command.hpp"
#include "config.hpp"
#include "fleshy.hpp"
#include "gpio.hpp"
#include "ossm/OSSM.h"
#include "ossm/advanced_penetration/advanced_penetration.h"
#include "patterns.hpp"
#include "services/led.h"
#include "services/UserConfig.h"
#include "state.hpp"
#include "wifi.hpp"

// Define the global variables
NimBLEServer* pServer = nullptr;

static long lostConnectionTime = 0;
static int speedOnLostConnection = 0;
// Duration for speed ramp to zero
static const unsigned long RAMP_DURATION_MS = 2000;  

double easeInOutSine(double t) {
    return 0.5 * (1 + sin(3.1415926 * (t - 0.5)));
}

/** Handler class for server actions */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI("NIMBLE", "Client connected: %s", connInfo.getAddress().toString().c_str());
        ESP_LOGI("NIMBLE", "Connection count: %d", pServer->getConnectedCount());
        lostConnectionTime = 0;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        ESP_LOGI("NIMBLE", "Client disconnected: %s, reason: %d", connInfo.getAddress().toString().c_str(), reason);
        ESP_LOGI("NIMBLE", "Connection count: %d", pServer->getConnectedCount());

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
            if (stateMachine->is("menu.idle"_s) || stateMachine->is("error.idle"_s) || stateMachine->is("wifi.idle"_s)) {
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

        NimBLEService* pSvc = pServer->getServiceByUUID(LEGACY_SERVICE_UUID);
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
    NimBLEService* ossmService = pServer->createService(OSSM_SERVICE_UUID);
    NimBLEService* lecacyService = pServer->createService(LEGACY_SERVICE_UUID);
    NimBLEService* infoService = pServer->createService(DEVICE_INFO_SERVICE_UUID);
    NimBLEService* fleshyService = pServer->createService(FLESHY_UUID);

    // OSSM Service Items
    advanced_penetration::initNimble(ossmService);
    initHomingTypeConfigCharacteristic(ossmService, NimBLEUUID(HOMING_UUID));
    initRailLengthConfigCharacteristic(ossmService, NimBLEUUID(LENGTH_UUID));
    initReHomeConfigCharacteristic(ossmService, NimBLEUUID(REHOME_UUID));
    initDirectionConfigCharacteristic(ossmService, NimBLEUUID(INVERT_UUID));
    initRenameConfigCharacteristic(ossmService, NimBLEUUID(RENAME_UUID));
    initGPIOCharacteristic(ossmService, NimBLEUUID(GPIO_UUID));
    initUpdateCharacteristic(ossmService, NimBLEUUID(UPDATE_UUID));
    initWiFiConfigCharacteristic(ossmService, NimBLEUUID(WIFI_UUID));
    initMaxAccelerationConfigCharacteristic(ossmService, NimBLEUUID(MACCEL_UUID));
    initMotorRPMConfigCharacteristic(ossmService, NimBLEUUID(MAXRPM_UUID));
    initMotorStepsConfigCharacteristic(ossmService, NimBLEUUID(STEPPR_UUID));
    initPulleyTeethConfigCharacteristic(ossmService, NimBLEUUID(PULLEY_UUID));
    initBeltPitchConfigCharacteristic(ossmService, NimBLEUUID(BPITCH_UUID));

    // Lecacy Service Items
    initCommandCharacteristic(lecacyService, NimBLEUUID(LEGACY_COMMAND_UUID));
    initStateCharacteristic(lecacyService, NimBLEUUID(CHARACTERISTIC_STATE_UUID));
    initSpeedKnobConfigCharacteristic(lecacyService, NimBLEUUID(SPEED_KNOB_UUID));
    initLatencyCompensationConfigCharacteristic(lecacyService, NimBLEUUID(BUFFER_UUID));
    initPatternsCharacteristic(lecacyService, NimBLEUUID(PATTERN_UUID));
    initPatternDataCharacteristic(lecacyService, NimBLEUUID(PATTERN_DATA_UUID));

    //Device info Service Items
    NimBLECharacteristic* manufacturer = infoService->createCharacteristic(MANUFACTURER_NAME_UUID, NIMBLE_PROPERTY::READ);
    manufacturer->setValue(ui::strings::kinkyMakers);
    NimBLECharacteristic* model = infoService->createCharacteristic(MODEL_UUID, NIMBLE_PROPERTY::READ);
    model->setValue(ui::strings::deviceName);
    NimBLECharacteristic* version = infoService->createCharacteristic(FIRMWARE_VERSION_UUID, NIMBLE_PROPERTY::READ);
    version->setValue(VERSION);

    initFleshyCharacteristic(fleshyService);

    // Update advertising to include new services
    NimBLEAdvertising* advert = NimBLEDevice::getAdvertising();
    advert->setName(UserConfig::getDeviceName());
    advert->addServiceUUID(ossmService->getUUID());
    advert->addServiceUUID(lecacyService->getUUID());
    advert->addServiceUUID(infoService->getUUID());
    advert->addServiceUUID(fleshyService->getUUID());
    advert->enableScanResponse(true);
    advert->start();

    xTaskCreatePinnedToCore(
        nimbleLoop, "nimbleLoop", 5 * configMINIMAL_STACK_SIZE, pServer,
        configMAX_PRIORITIES - 1, nullptr, Tasks::stepperCore);
}
