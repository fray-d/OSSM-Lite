#include "advanced_penetration.h"

#include "advanced_penetration_structs.h"
#include "advanced_penetration_ui.h"
#include "constants/Config.h"
#include "ossm/state/calibration.h"
#include "ossm/state/menu.h"
#include "services/communication/nimble.h"
#include "services/led.h"
#include "services/stepper.h"
#include "services/tasks.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {

    NimBLECharacteristic* statusNotifier = nullptr;

    static void startAdvancedPenetrationMotionTask(void* pvParameters) {
        u32_t strokeCount = 1;
        while (stateMachine->is("advancedPenetration"_s) || stateMachine->is("advancedPenetration.idle"_s) ||
               stateMachine->is("advancedPenetration.presets"_s)) {
            if (currentSettings.changed && statusNotifier != nullptr) {
                statusNotifier->setValue(currentSettings.getStatus());
                statusNotifier->notify();
            }
            if (currentSettings.speed.value == 0.0) {
                stepper->stopMove();
                currentSettings.changed = false;
                vTaskDelay(100);
                continue;
            }
            vTaskDelay(1);
            if (!currentSettings.changed && stepper->isRunning()) {
                continue;
            }
            if (!stepper->isRunning()) {
                strokeCount++;
            }
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
            if (acceleration > stepper->getAcceleration() || !stepper->isRunning()) {
                stepper->setAcceleration(acceleration);
                stepper->applySpeedAcceleration();
            }
            if (!stepper->isRunning()) {
                stepper->moveTo(targetPosition, false);
            }
        }
        currentSettings.speed.value = 0;
        vTaskDelete(nullptr);
    }

    class AdvancedCommandCallbacks : public NimBLECharacteristicCallbacks {
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            if (!(stateMachine->is("advancedPenetration"_s) || stateMachine->is("advancedPenetration.idle"_s) ||
                  stateMachine->is("advancedPenetration.presets"_s))) {
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
    } apComandCB;

    class AdvancedConfigCallbacks : public NimBLECharacteristicCallbacks {
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            pCharacteristic->setValue(currentSettings.getStatus(true));
        }
    } apConfigCB;

    class AdvancedPresetCallbacks : public NimBLECharacteristicCallbacks {
        u32_t lastPresetCommand = millis();
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            u32_t currentTime = millis();
            if (currentTime - lastPresetCommand > 1000) {
                String cmd = pCharacteristic->getValue();
                String value = cmd.substring(1);
                cmd = cmd.substring(0, 1);
                if (cmd == ">") {
                    savePreset(value, currentSettings.getPreset());
                } else if (cmd == "<") {
                    deletePreset(value);
                } else if (cmd == ":") {
                    currentSettings.processStringCommand(readPresetValueCommand(value));
                } else if (cmd == "^") {
                    factoryReset();
                    repopulatePresets();
                }
                pCharacteristic->setValue(getPreset("names"));
                pulseForCommunication();
            } else {
                ESP_LOGI("AP", "Skipping commands sent too soon.");
            }
            lastPresetCommand = currentTime;
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            std::string names = getPreset("names");
            pCharacteristic->setValue(getPreset("names"));
        }
    } apPresetCB;

    class AdvancedStatusCallbacks : public NimBLECharacteristicCallbacks {
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            pCharacteristic->setValue(currentSettings.getStatus());
        }
    } apStatusCB;

    NimBLECharacteristic* initCharacteristic(NimBLEService* pService, std::string uuid, uint32_t properties,
                                             NimBLECharacteristicCallbacks* callbacks) {
        NimBLECharacteristic* pChar = pService->createCharacteristic(NimBLEUUID(uuid), properties);
        pChar->setCallbacks(callbacks);
        return pChar;
    }

    NimBLEService* initNimble() {
        initPresets();
        NimBLEService* as = pServer->createService(ADVANCED_SERVICE_UUID);
        initCharacteristic(as, CHARACTERISTIC_ADVANCED_CONTROL_UUID,
                           NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR, &apComandCB);
        initCharacteristic(as, CHARACTERISTIC_ADVANCED_CONFIG_UUID, NIMBLE_PROPERTY::READ, &apConfigCB);
        initCharacteristic(as, CHARACTERISTIC_ADVANCED_PRESETS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, &apPresetCB);
        statusNotifier =
            initCharacteristic(as, CHARACTERISTIC_ADVANCED_STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, &apStatusCB);
        return as;
    }

    void startAdvancedPenetration() {
        int stackSize = 10 * configMINIMAL_STACK_SIZE;
        currentSettings.lastStatus = ControlStatus::STATUS_COUNT;
        currentSettings.status = ControlStatus::BASE_MENU;
        encoder.setBoundaries(0, BaseControls::BASE_COUNT * 6 - 1, true);
        encoder.setEncoderValue(0);

        if (isDisplayAvailable()) {
            xTaskCreatePinnedToCore(startAdvancedPenetrationUITask, "startAdvancedPenetrationTask", stackSize, nullptr,
                                    configMAX_PRIORITIES - 1, &Tasks::runAdvancedPenetrationTaskH, Tasks::operationTaskCore);
        }

        xTaskCreatePinnedToCore(startAdvancedPenetrationMotionTask, "startAdvancedPenetrationMotionTask", stackSize, nullptr,
                                configMAX_PRIORITIES - 1, nullptr, Tasks::operationTaskCore);
    }
}