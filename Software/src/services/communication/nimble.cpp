#include "nimble.h"

#include "constants/Version.h"
#include "command.hpp"
#include "config.hpp"
#include "fleshy.hpp"
#include "gpio.hpp"
#include "ossm/OSSM.h"
#include "ossm/advanced_penetration/advanced_penetration.h"
#include "patterns.hpp"
#include "services/led.h"
#include "services/tasks.h"
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
        ble_gattc_exchange_mtu(connInfo.getConnHandle(), nullptr, nullptr);
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

void notifyValue(const char* service, const char* uuid, String value) {
    NimBLEService* ossmService = pServer->getServiceByUUID(service);
    if (!ossmService) {
        ESP_LOGE("NIMBLE", "Notifing service is not available?");
        return;
    }
    NimBLECharacteristic* noteChar = ossmService->getCharacteristic(uuid);
    if (!noteChar) {
        ESP_LOGE("NIMBLE", "Notifying characteristic is not available?");
        return;
    }
    if (value != noteChar->getValue()) {
        ESP_LOGD("NIMBLE", "Notifying: %s", value.c_str());
        noteChar->setValue(value);
        noteChar->notify();
        pulseForCommunication();
    }
}

void nimbleLoop(void* pvParameters) {
    String lastState = "";
    int lastConnCount = 0;
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

        // mannage message queue
        while (!messageQueue.empty()) {
            String cmd = messageQueue.front();
            messageQueue.pop();
            ossm->ble_click(cmd);
            notifyValue(LEGACY_SERVICE_UUID, LEGACY_COMMAND_UUID, "Ok:" + cmd);
            // Trigger LED communication pulse for command processing
            pulseForCommunication();
            vTaskDelay(1);
        }

        String fingerprint = ossm->getStateFingerprint();
        if (fingerprint == lastState) {
            vTaskDelay(100);
            continue;
        }
        lastState = fingerprint;

        notifyValue(OSSM_MOTION_SERVICE_UUID, SPEED_UUID, String(settings.speed));
        notifyValue(OSSM_MOTION_SERVICE_UUID, MAXDEP_UUID, String(settings.maxPosition));
        notifyValue(OSSM_MOTION_SERVICE_UUID, MINDEP_UUID, String(settings.minPosition));
        notifyValue(OSSM_MOTION_SERVICE_UUID, SENSAT_UUID, String(settings.sensation));
        notifyValue(OSSM_MOTION_SERVICE_UUID, SENPAT_UUID, String((int)settings.pattern));
        
        String currentState = ossm->getCurrentState();
        ESP_LOGD("NIMBLE", "State changed to: %s", currentState.c_str());
        notifyValue(LEGACY_SERVICE_UUID, STATE_UUID, currentState);
        // Trigger LED communication pulse for state update
        pulseForCommunication();
        vTaskDelay(1);
    }
}

void initNimble() {
    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init(UserConfig::getDeviceName());
    NimBLEDevice::setMTU(512);

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_SC);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
 
    // Create Service
    NimBLEService* ossmConfigService = pServer->createService(OSSM_CONFIG_SERVICE_UUID);
    NimBLEService* ossmMotionService = pServer->createService(OSSM_MOTION_SERVICE_UUID);
    NimBLEService* ossmAdvancedService = pServer->createService(OSSM_ADVANCED_SERVICE_UUID);
    NimBLEService* lecacyService = pServer->createService(LEGACY_SERVICE_UUID);
    NimBLEService* infoService = pServer->createService(DEVICE_INFO_SERVICE_UUID);
    NimBLEService* fleshyService = pServer->createService(FLESHY_UUID);

    // OSSM Service Items
    advanced_penetration::initNimble(ossmAdvancedService);
    initCharacteristic(ossmConfigService, NimBLEUUID(HOMING_UUID), &homingTypeConfigCallbacks, "Homing Type");
    initCharacteristic(ossmConfigService, NimBLEUUID(LENGTH_UUID), &railLengthConfigCallbacks, "Rail Length");
    initCharacteristic(ossmConfigService, NimBLEUUID(REHOME_UUID), &ReHomeConfigCallbacks, "Home Between Modes");
    initCharacteristic(ossmConfigService, NimBLEUUID(INVERT_UUID), &directionConfigCallbacks, "Rail Direction");
    initCharacteristic(ossmConfigService, NimBLEUUID(RENAME_UUID), &renameConfigCallbacks, "Rename Device");
    initCharacteristic(ossmConfigService, NimBLEUUID(GPIO_UUID),   &gpioCallbacks, "GPIO");
    initUpdateCharacteristic(ossmConfigService, NimBLEUUID(UPDATE_UUID));
    initWiFiConfigCharacteristic(ossmConfigService, NimBLEUUID(WIFI_UUID));
    initCharacteristic(ossmConfigService, NimBLEUUID(MACCEL_UUID), &maxAccelerationConfigCallbacks, "Max Acceleration");
    initCharacteristic(ossmConfigService, NimBLEUUID(MAXRPM_UUID), &motorRPMConfigCallbacks, "Max Motor RPM");
    initCharacteristic(ossmConfigService, NimBLEUUID(STEPPR_UUID), &motorStepsConfigCallbacks, "Steps Per Revolution");
    initCharacteristic(ossmConfigService, NimBLEUUID(PULLEY_UUID), &pulleyTeethConfigCallbacks, "Pulley Teeth");
    initCharacteristic(ossmConfigService, NimBLEUUID(BPITCH_UUID), &beltPitchConfigCallbacks, "Belt Pitch");
    initCharacteristic(ossmConfigService, NimBLEUUID(SENSOR_UUID), &sensorLimitConfigCallbacks, "Current Sensor Limit");
    initCharacteristic(ossmConfigService, NimBLEUUID(SPDCRV_UUID), &speedCurveConfigCallbacks, "Speed Curve");
    initCharacteristic(ossmConfigService, NimBLEUUID(SPDHOM_UUID), &homingSpeedConfigCallbacks, "Homing Speed");

    //Shared OSSM Settings
    initFloatCharacteristic(ossmMotionService, NimBLEUUID(SPEED_UUID),  &speedCallbacks, "Speed", 0);
    initFloatCharacteristic(ossmMotionService, NimBLEUUID(MAXDEP_UUID), &maxDepthCallbacks, "Max Depth", 1);
    initFloatCharacteristic(ossmMotionService, NimBLEUUID(MINDEP_UUID), &minDepthCallbacks, "Min Depth", 2);
    initFloatCharacteristic(ossmMotionService, NimBLEUUID(SENSAT_UUID), &sensationCallbacks, "Sensation", 3);
    initPatternCommandCharacteristic(ossmMotionService, NimBLEUUID(SENPAT_UUID), &strokeEnginePatternCallbacks);
    initSimplePatternCharacteristic(ossmMotionService, NimBLEUUID(SENPTL_UUID));
    initStreamCharacteristic(ossmMotionService, NimBLEUUID(STREAM_UUID), &streamCallbacks, "Stream Input", 50);

    // Lecacy Service Items
    initCommandCharacteristic(lecacyService, NimBLEUUID(LEGACY_COMMAND_UUID));
    initStateCharacteristic(lecacyService, NimBLEUUID(STATE_UUID));
    initCharacteristic(lecacyService, NimBLEUUID(SPEED_KNOB_UUID), &speedKnobConfigCallbacks, "");
    initCharacteristic(lecacyService, NimBLEUUID(BUFFER_UUID),&latencyCompensationConfigCallbacks, "");
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
    advert->addServiceUUID(ossmMotionService->getUUID());
    advert->addServiceUUID(ossmConfigService->getUUID());
    advert->addServiceUUID(ossmAdvancedService->getUUID());
    advert->addServiceUUID(lecacyService->getUUID());
    advert->addServiceUUID(infoService->getUUID());
    advert->addServiceUUID(fleshyService->getUUID());
    advert->enableScanResponse(true);
    advert->start();

    xTaskCreatePinnedToCore(
        nimbleLoop, "nimbleLoop", 5 * configMINIMAL_STACK_SIZE, pServer,
        configMAX_PRIORITIES - 1, nullptr, Tasks::stepperCore);
}
