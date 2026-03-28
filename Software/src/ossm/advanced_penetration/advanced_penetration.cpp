#include "advanced_penetration.h"

#include "constants/Config.h"
#include "utils/analog.h"
#include "ui.h"
#include "services/display.h"
#include "ossm/state/calibration.h"
#include "ossm/state/state.h"
#include "services/encoder.h"
#include "services/stepper.h"
#include "services/tasks.h"
#include "Strings.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {

static const char depth[] PROGMEM = "Depth";
static const char speed[] PROGMEM = "Speed";
static const char accel[] PROGMEM = "Accel";
static const char modifier[] PROGMEM = "Modifier";
static const char amplitude[] PROGMEM = "Amplitude";
static const char in_step[] PROGMEM = "Rise";
static const char in_wait[] PROGMEM = "Linger";
static const char out_step[] PROGMEM = "Fall";
static const char out_wait[] PROGMEM = "Dwell";

enum MenuStatus {
    BASE_MENU,
    BASE_VALUE,
    MODIFIER_MENU,
    MODIFIER_VALUE,
    STATUS_COUNT
};

enum BaseMenu {
    SPEED_1,
    ACCEL_1,
    DEPTH_1,
    SPEED_2,
    ACCEL_2,
    DEPTH_2,
    BASE_COUNT
};

enum ModifierMenu {
    MODIFIER_BACK,
    AMPLITUDE,
    IN_STEP,
    IN_WAIT,
    OUT_STEP,
    OUT_WAIT,
    MODIFIER_COUNT
};

struct AdvancedModifier;

struct Control {
    u8_t value;
    u8_t minValue = 0;
    u8_t maxValue = 100;
    float getRampValue(float exp){
        return pow(1 - pow(1 - (value/100.0), exp), 1 / exp);
    }
};

struct ModifierControl : public Control {
    ModifierMenu id;
};

struct AdvancedModifier {
    ModifierControl amplitude = {0, 0, 100, ModifierMenu::AMPLITUDE};
    ModifierControl inStep = {1, 0, 100, ModifierMenu::IN_STEP};
    ModifierControl inWait = {0, 0, 100, ModifierMenu::IN_WAIT};
    ModifierControl outStep = {1, 0, 100, ModifierMenu::OUT_STEP};
    ModifierControl outWait = {0, 0, 100, ModifierMenu::OUT_WAIT};
    int stepCount() {
        return inStep.value + inWait.value + outStep.value + outWait.value;
    }
    bool active() {
        return amplitude.value > 0;
    }
};

struct AdvancedControl : Control {
    BaseMenu id;
    AdvancedModifier* modifier;
    u8_t getModifiedValue(int strokeCount){
        if (modifier == nullptr) {
            return value;
        }
        int8_t difference = value - minValue;
        if (id == BaseMenu::DEPTH_2){
            difference = value - maxValue;
        }
        int cycle = (strokeCount / 2) % modifier->stepCount();
        int8_t usableDifference = difference * modifier->amplitude.value/100.0;
        if (cycle < modifier->inStep.value){
            int8_t slice = usableDifference/modifier->inStep.value * (cycle + 1);
            return value - usableDifference + slice;
        } else {
            cycle -= modifier->inStep.value;
        }
        if (cycle < modifier->inWait.value) {
            return value;
        } else {
            cycle -= modifier->inWait.value;
        }
        if (cycle < modifier->outStep.value) {
            int8_t slice = usableDifference/modifier->outStep.value * (cycle + 1);
            return value - slice;
        } else {
            cycle -= modifier->outStep.value;
        }
        if (cycle < modifier->outWait.value) {
            return value - usableDifference;
        }
        return value;
    }
    float getNormalizedModifiedValue(int strokeCount) {
        return getModifiedValue(strokeCount) / 100.0;
    }
    float getRampedModifiedValue(int strokeCount, float exp) {
        return pow(1 - pow(1 - getNormalizedModifiedValue(strokeCount),exp), 1 / exp);
    }
};

struct AdvancedSettings {
    AdvancedControl speed = {0,0,100, BaseMenu::BASE_COUNT};
    AdvancedControl inSpeed = {100,1,100, BaseMenu::SPEED_1};
    AdvancedControl outSpeed = {100,1,100, BaseMenu::SPEED_2};
    AdvancedControl maxDepth = {10,0,100, BaseMenu::DEPTH_1};
    AdvancedControl minDepth = {0,0,100, BaseMenu::DEPTH_2};
    AdvancedControl inAcceleration = {1,1,100, BaseMenu::ACCEL_1};
    AdvancedControl outAcceleration = {1,1,100, BaseMenu::ACCEL_2};

    MenuStatus status = MenuStatus::BASE_MENU;
    MenuStatus lastStatus = MenuStatus::STATUS_COUNT;
    BaseMenu baseMenu = BaseMenu::DEPTH_1;
    ModifierMenu modifierMenu = ModifierMenu::AMPLITUDE;
    bool changed = false;
} currentSettings;

// void textControl(AdvancedControl& a, int height) {
//     if (currentSettings.selectedModifierControl == a.id) {
//         if (currentSettings.lastControl != a.id) {
//             encoder.setBoundaries(a.minValue, a.maxValue, false);
//             encoder.setEncoderValue(a.value);
//             currentSettings.lastControl = a.id;
//         }
//         a.value = constrain(encoder.readEncoder(),a.minValue,a.maxValue);
//     }
//     String controlText = a.name +": " + String(uint8_t(a.value));
//     uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
//     display.drawUTF8(128 - stringWidth, height, controlText.c_str());
// }

void centeredText(String controlText, u8_t x, u8_t y) {
    uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
    display.drawUTF8(x - stringWidth/2, y, controlText.c_str());
}

void updateControl(AdvancedControl& a, u8_t minVal, u8_t maxVal, u8_t x, u8_t y) {
    if (currentSettings.status == MenuStatus::MODIFIER_MENU){
        return;
    }
    display.setFont(u8g2_font_helvR08_tf);
    if (currentSettings.baseMenu == a.id) {
        display.drawFrame(x-13,y-11,26,14);
        if (currentSettings.status == MenuStatus::BASE_VALUE){
            if (currentSettings.lastStatus != currentSettings.status) {
                encoder.setBoundaries(minVal, maxVal, false);
                encoder.setEncoderValue(a.value);
            }
            display.setFont(u8g2_font_helvB08_tf);
            a.value = constrain(encoder.readEncoder(),minVal, maxVal);
            a.minValue = minVal;
            a.maxValue = maxVal;
        }
        currentSettings.lastStatus = currentSettings.status;
    }
    if (encoder.readEncoder()/3 >= BaseMenu::BASE_COUNT && currentSettings.status == MenuStatus::BASE_MENU) {
        if (a.modifier != nullptr && a.modifier->active()) {
            display.setFont(u8g2_font_helvB08_tf);
        }
        centeredText("Mod", x, y);
        return;
    }
    centeredText(String(a.value), x, y);
}

bool isInCorrectState() {
    return stateMachine->is("advancedPenetration"_s)
        || stateMachine->is("advancedPenetration.idle"_s)
        || stateMachine->is("advancedPenetration.modifier"_s);
};

static void startAdvancedPenetrationTask(void *pvParameters) {
    encoder.setAcceleration(0);
    u8_t lastEncoder = 0;

    while (isInCorrectState()) {
        u8_t speedValue = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        u8_t encoderValue = encoder.readEncoder();
        if ( abs(speedValue - currentSettings.speed.value) > 1
                    || speedValue == 0
                    || encoderValue != lastEncoder 
                    || currentSettings.status != currentSettings.lastStatus) {
            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                currentSettings.speed.value = speedValue;
                lastEncoder = encoderValue;

                String headerText = ui::strings::advancedPenetration;
                setHeader(headerText);
                clearPage(true);

                ui::drawShape::settingBar(display.getU8g2(),"", currentSettings.speed.value);

                int item = floor(encoderValue / 3);

                switch (currentSettings.status) {
                    case MenuStatus::BASE_MENU:
                        item = item % BaseMenu::BASE_COUNT;
                        currentSettings.baseMenu = BaseMenu(item);
                    case MenuStatus::BASE_VALUE:
                        display.setFont(u8g2_font_helvR08_tf);
                        centeredText(speed, 30, 24);
                        centeredText(accel, 70, 24);
                        centeredText(depth, 110, 24);
                        break;
                    case MenuStatus::MODIFIER_MENU:
                        item = item % ModifierMenu::MODIFIER_COUNT;
                        currentSettings.modifierMenu = ModifierMenu(item);
                        Serial.println(currentSettings.modifierMenu);
                        break;
                    default:
                        break;
                }

                updateControl(currentSettings.inSpeed,1, 100, 30, 40);
                updateControl(currentSettings.outSpeed,1, 100, 30, 58);
                updateControl(currentSettings.inAcceleration, 1, 100, 70, 40);
                updateControl(currentSettings.outAcceleration, 1, 100, 70, 58);
                updateControl(currentSettings.maxDepth, currentSettings.minDepth.value, 100, 110, 40);
                updateControl(currentSettings.minDepth, 0, currentSettings.maxDepth.value, 110, 58);

                currentSettings.changed = true;

                refreshPage(true);
                xSemaphoreGive(displayMutex);
            }
        }
        vTaskDelay(100);
    }
    vTaskDelete(nullptr);
}

static void startAdvancedPenetrationMotionTask(void *pvParameters) {
    int strokeCount = 0;
    while (isInCorrectState()) {
        if (currentSettings.speed.value == 0.0) {
            stepper->stopMove();
            vTaskDelay(100);
            continue;
        }
        if (!stepper->isRunning()) {
            strokeCount ++;
        }
        if (currentSettings.changed || !stepper->isRunning()) {
            currentSettings.changed = false;
            float speed = Config::Driver::maxSpeedMmPerSecond * (1_mm) * currentSettings.speed.getRampValue(0.8);
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
            u32_t maxAccel = min(minAccel * 5,u32_t(Config::Driver::maxAcceleration * (1_mm)));
            u32_t accelDifference = maxAccel - minAccel;
            u32_t acceleration = minAccel;
            if (strokeCount % 2 == 0) {
                acceleration += accelDifference * currentSettings.inAcceleration.getRampedModifiedValue(strokeCount,0.8);
            } else {
                acceleration += accelDifference * currentSettings.outAcceleration.getRampedModifiedValue(strokeCount, 0.8);
            }
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
    vTaskDelete(nullptr);
}

void startAdvancedPenetration() {
    int stackSize = 10 * configMINIMAL_STACK_SIZE;
    currentSettings.lastStatus = MenuStatus::STATUS_COUNT;
    currentSettings.status = MenuStatus::BASE_MENU;
    encoder.setBoundaries(0,BaseMenu::BASE_COUNT * 6 - 1, true);
    encoder.setEncoderValue(0);

    xTaskCreatePinnedToCore(startAdvancedPenetrationTask,
                            "startAdvancedPenetrationTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1,
                            &Tasks::runAdvancedPenetrationTaskH,
                            Tasks::operationTaskCore);

    xTaskCreatePinnedToCore(startAdvancedPenetrationMotionTask, 
                            "startAdvancedPenetrationMotionTask", stackSize, nullptr,
                            configMAX_PRIORITIES - 1, nullptr,
                            Tasks::operationTaskCore);
}

void advancedClick() {
    int c = 100;
    bool loop = true;
    switch (currentSettings.status) {
        case MenuStatus::BASE_MENU:
            if (encoder.readEncoder()/3 >= BaseMenu::BASE_COUNT) {
                currentSettings.status = MenuStatus::MODIFIER_MENU;
                c = ModifierMenu::MODIFIER_COUNT * 3;
            } else {
                currentSettings.status = MenuStatus::BASE_VALUE;
                loop = false;
            }
            break;
        case MenuStatus::BASE_VALUE:
            currentSettings.status = MenuStatus::BASE_MENU;
            encoder.setEncoderValue(currentSettings.baseMenu*3-1);
            c = BaseMenu::BASE_COUNT * 6;
            break;
        case MenuStatus::MODIFIER_MENU:
            currentSettings.status = MenuStatus::BASE_MENU;
            encoder.setEncoderValue(currentSettings.baseMenu*6-1);
            c = BaseMenu::BASE_COUNT * 6;
            break;
        case MenuStatus::MODIFIER_VALUE:
            currentSettings.status = MenuStatus::MODIFIER_MENU;
            c = ModifierMenu::MODIFIER_COUNT * 3;
            break;
        default:
            break;
    }
    encoder.setBoundaries(0, c - 1, loop);
    Serial.println(currentSettings.status);
}

void advancedDoubleClick() { 
}

}