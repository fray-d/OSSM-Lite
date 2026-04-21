#include "advanced_penetration.h"
#include "advanced_penetration_structs.h"
#include "advanced_penetration_ui.h"
#include "constants/Config.h"
#include "ossm/state/calibration.h"
#include "ossm/state/menu.h"
#include "services/communication/nimble.h"
#include "services/stepper.h"
#include "services/tasks.h"
#include "services/led.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {

NimBLECharacteristic* advancedCharacteristic = nullptr;;

static void startAdvancedPenetrationMotionTask(void *pvParameters) {
    u32_t strokeCount = 0;
    while (stateMachine->is("advancedPenetration"_s) 
                    || stateMachine->is("advancedPenetration.idle"_s)
                    || stateMachine->is("advancedPenetration.presets"_s)) {
        if (currentSettings.changed && advancedCharacteristic != nullptr) {
            advancedCharacteristic->setValue(currentSettings.getStatus());
            advancedCharacteristic->notify();
        }
        if (currentSettings.speed.value == 0.0) {
            stepper->stopMove();
            currentSettings.changed = false;
            vTaskDelay(100);
            continue;
        }
        if (!stepper->isRunning()) {
            strokeCount ++;
        }
        if (currentSettings.changed || !stepper->isRunning()) {
            currentSettings.changed = false;
            float speed = Config::Driver::maxSpeedMmPerSecond * (1_mm) * currentSettings.speed.getRampValue();
            int32_t targetPosition = -calibration.measuredStrokeSteps;
            if (strokeCount % 2 == 0) {
                speed = speed * currentSettings.inSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.maxDepth.getNormalizedModifiedValue(strokeCount);
            } else {
                speed = speed * currentSettings.outSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.minDepth.getNormalizedModifiedValue(strokeCount);
            }
            stepper->setSpeedInHz(speed);
            stepper->applySpeedAcceleration();

            u32_t distance = abs(targetPosition - stepper->getCurrentPosition());
            u32_t minAccel = speed / (distance / speed);          
            u32_t acceleration = minAccel;
            if (strokeCount % 2 == 0) {
                acceleration += minAccel * 9 * currentSettings.inAcceleration.getRampedModifiedValue(0.6, strokeCount);
            } else {
                acceleration += minAccel * 9 * currentSettings.outAcceleration.getRampedModifiedValue(0.6, strokeCount);
            }
            acceleration = min(acceleration, u32_t(Config::Driver::maxAcceleration * (1_mm)));
            if (acceleration > stepper->getAcceleration() || !stepper->isRunning()){
                stepper->setAcceleration(acceleration);
                stepper->applySpeedAcceleration();
            }
            if (!stepper->isRunning()) {
                stepper->moveTo(targetPosition,false);
            }
        }
        vTaskDelay(1);
    }
    currentSettings.speed.value = 0;
    vTaskDelete(nullptr);
}

class AdvancedCommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        if (!(stateMachine->is("advancedPenetration"_s) 
                        || stateMachine->is("advancedPenetration.idle"_s)
                        || stateMachine->is("advancedPenetration.presets"_s))) {
            stateMachine->process_event(LongPress{});
            menuState.currentOption = Menu::AdvancedPenetration;
            if (stateMachine != nullptr) {
                stateMachine->process_event(ButtonPress{});
            }
        } 
        String cmd = pCharacteristic->getValue();
        bool saved = currentSettings.processStringCommand(cmd);
        if (!saved) {
            ESP_LOGD("Advanced", "Command failed: %s", String(cmd));
            pCharacteristic->setValue("fail:" + String(cmd.c_str()));
            return;
        }
        pulseForCommunication();
    }
} advancedCommandCallbacks;

class AdvancedConfigCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(currentSettings.getStatus(true));
    }
} advancedConfigCallbacks;

class AdvancedStatusCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        pCharacteristic->setValue(currentSettings.getStatus());
    }
} advancedStatusCallbacks;

NimBLECharacteristic* initAdvancedCommandCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR);
    pChar->setCallbacks(&advancedCommandCallbacks);
    return pChar;
}

NimBLECharacteristic* initAdvancedConfigCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ,10000);
    pChar->setCallbacks(&advancedConfigCallbacks);
    return pChar;
}

NimBLECharacteristic* initAdvancedStatusCharacteristic(NimBLEService* pService, NimBLEUUID uuid) {
    NimBLECharacteristic* pChar = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,1000);
    pChar->setCallbacks(&advancedStatusCallbacks);
    return pChar;
}

NimBLEService* initNimble() {
    NimBLEService* advancedService = pServer->createService(ADVANCED_SERVICE_UUID);
    initAdvancedCommandCharacteristic(advancedService, NimBLEUUID(CHARACTERISTIC_ADVANCED_CONTROL_UUID));
    initAdvancedConfigCharacteristic(advancedService, NimBLEUUID(CHARACTERISTIC_ADVANCED_CONFIG_UUID));
    advancedCharacteristic = initAdvancedStatusCharacteristic(advancedService, NimBLEUUID(CHARACTERISTIC_ADVANCED_STATUS_UUID));
    advancedService->start();
    return advancedService;
}

void startAdvancedPenetration() {
    int stackSize = 10 * configMINIMAL_STACK_SIZE;
    currentSettings.lastStatus = ControlStatus::STATUS_COUNT;
    currentSettings.status = ControlStatus::BASE_MENU;
    encoder.setBoundaries(0,BaseControls::BASE_COUNT * 6 - 1, true);
    encoder.setEncoderValue(0);

    if(isDisplayAvailable()) {
        xTaskCreatePinnedToCore(startAdvancedPenetrationUITask,
                            "startAdvancedPenetrationTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1,
                            &Tasks::runAdvancedPenetrationTaskH,
                            Tasks::operationTaskCore);
    }

    xTaskCreatePinnedToCore(startAdvancedPenetrationMotionTask, 
                            "startAdvancedPenetrationMotionTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1, nullptr,
                            Tasks::operationTaskCore);
}
}