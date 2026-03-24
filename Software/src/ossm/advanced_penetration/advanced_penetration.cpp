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

enum AdvancedControls {
    MAX_DEPTH,
    MIN_DEPTH,
    IN_SPEED,
    OUT_SPEED,
    IN_ACCELERATION,
    OUT_ACCELERATION,
    Count,
    AMPLITUDE,
    IN_STEP,
    IN_WAIT,
    OUT_STEP,
    OUT_WAIT,
    Count2,
    SPEED //uncounted
};

struct AdvancedModifier;

struct AdvancedControl {
    AdvancedControls id;
    float value;
    String name;
    float minValue = 0;
    float maxValue = 100;
    AdvancedModifier* modifier;
    float getModifiedValue(int strokeCount);
    float getNormalizedModifiedValue(int strokeCount) {
        return getModifiedValue(strokeCount) / 100.0;
    }
    float getRampValue(float exp){
        return pow(1 - pow(1 - (value/100.0), exp), 1 / exp);
    }
    float getRampedModifiedValue(int strokeCount, float exp) {
        return pow(1 - pow(1 - getNormalizedModifiedValue(strokeCount),exp), 1 / exp);
    }
};

struct AdvancedModifier {
    AdvancedControl amplitude = {AdvancedControls::AMPLITUDE,0,"Amplitude"};
    AdvancedControl inStep = {AdvancedControls::IN_STEP,1,"In Steps",1,20};
    AdvancedControl inWait = {AdvancedControls::IN_WAIT,0,"Linger",0,20};
    AdvancedControl outStep = {AdvancedControls::OUT_STEP,1,"Out Steps",1,20};
    AdvancedControl outWait = {AdvancedControls::OUT_WAIT,0,"Dwell",0,20};
    int stepCount() {
        return inStep.value + inWait.value + outStep.value + outWait.value;
    }
};

float AdvancedControl::getModifiedValue(int strokeCount){
    if (modifier == nullptr) {
        return value;
    }
    float difference = value - minValue;
    if (id == AdvancedControls::MIN_DEPTH){
        difference = value - maxValue;
    }
    int cycle = (strokeCount / 2) % modifier->stepCount();
    float usableDifference = difference * modifier->amplitude.value/100.0;
    if (cycle < modifier->inStep.value){
        float slice = usableDifference/modifier->inStep.value * (cycle + 1);
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
        float slice = usableDifference/modifier->outStep.value * (cycle + 1);
        return value - slice;
    } else {
        cycle -= modifier->outStep.value;
    }
    if (cycle < modifier->outWait.value) {
        return value - usableDifference;
    }
    return value;
}

struct AdvancedSettings {
    AdvancedControl speed = {AdvancedControls::SPEED,0,"Speed"};
    AdvancedControl inSpeed = {AdvancedControls::IN_SPEED,100,"In Speed"};
    AdvancedControl outSpeed = {AdvancedControls::OUT_SPEED,100,"Out Speed"};
    AdvancedControl maxDepth = {AdvancedControls::MAX_DEPTH,10,"Max Depth"};
    AdvancedControl minDepth = {AdvancedControls::MIN_DEPTH,0,"Min Depth"};
    AdvancedControl inAcceleration = {AdvancedControls::IN_ACCELERATION, 1, "In Acceleration"};
    AdvancedControl outAcceleration = {AdvancedControls::OUT_ACCELERATION, 1, "Out Acceleration"};
    AdvancedControls selectedControl = AdvancedControls::MAX_DEPTH;
    AdvancedControls selectedModifierControl = AdvancedControls::AMPLITUDE;
    bool changed = false;
} currentSettings;

uint8_t controlPosition;
AdvancedControls lastControl = AdvancedControls::SPEED;

void textControl(AdvancedControl& a, int height) {
    if (currentSettings.selectedModifierControl == a.id) {
        if (lastControl != a.id) {
            encoder.setBoundaries(a.minValue, a.maxValue, false);
            encoder.setEncoderValue(a.value);
            lastControl = a.id;
        }
        a.value = constrain(encoder.readEncoder(),a.minValue,a.maxValue);
    }
    String controlText = a.name +": " + String(uint8_t(a.value));
    uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
    display.drawUTF8(128 - stringWidth, height, controlText.c_str());
}

void updateControl(AdvancedControl& a, float minVal = 0, float maxVal = 100) {
    if(stateMachine->is("advancedPenetration.modifier"_s)) {
        if (a.modifier == nullptr) {
            a.modifier = new AdvancedModifier;
        }
        if (currentSettings.selectedControl == a.id) {
            textControl(a.modifier->amplitude, 24);
            textControl(a.modifier->inStep, 34);
            textControl(a.modifier->inWait, 44);
            textControl(a.modifier->outStep, 54);
            textControl(a.modifier->outWait, 64);
        }
    } else {
        if (currentSettings.selectedControl == a.id) {
            if (lastControl != a.id) {
                encoder.setBoundaries(minVal, maxVal, false);
                encoder.setEncoderValue(a.value);
                lastControl = a.id;
            } 
            a.value = constrain(encoder.readEncoder(),minVal, maxVal);
            a.minValue = minVal;
            a.maxValue = maxVal;
            String controlText = a.name;
            uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
            display.drawUTF8(128 - stringWidth, 64, controlText.c_str());
            controlPosition += 3;
            ui::drawShape::settingBar(display.getU8g2(),"",a.value, controlPosition, 10, ui::Alignment::RIGHT_ALIGNED, 0, 40);
            controlPosition -= 15;
        } else {
            ui::drawShape::settingBarSmall(display.getU8g2(),a.value, controlPosition, 10, 40);
            controlPosition -= 5;
        }
    }
}

bool isInCorrectState() {
    return stateMachine->is("advancedPenetration"_s)
        || stateMachine->is("advancedPenetration.idle"_s)
        || stateMachine->is("advancedPenetration.modifier"_s);
};

static void startAdvancedPenetrationTask(void *pvParameters) {
    encoder.setAcceleration(10);
    float lastEncoder = 0;

    while (isInCorrectState()) {
        float speedValue = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        float encoderValue = encoder.readEncoder();
        if ( abs(speedValue - currentSettings.speed.value) > 1
                    || speedValue == 0
                    || encoderValue != lastEncoder 
                    || currentSettings.selectedControl != lastControl) {
            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                currentSettings.speed.value = speedValue;
                lastEncoder = encoderValue;

                String headerText = ui::strings::advancedPenetration;
                setHeader(headerText);
                clearPage(true);

                ui::drawShape::settingBar(display.getU8g2(),"", currentSettings.speed.value);
                controlPosition = 125;

                //Reverse order... right to left.
                updateControl(currentSettings.outAcceleration, 1);
                updateControl(currentSettings.inAcceleration, 1);
                updateControl(currentSettings.outSpeed,1);
                updateControl(currentSettings.inSpeed,1);
                updateControl(currentSettings.minDepth, 0, currentSettings.maxDepth.value);
                updateControl(currentSettings.maxDepth, currentSettings.minDepth.value);

                currentSettings.changed = true;

                refreshPage(true);
                xSemaphoreGive(displayMutex);
            }
        }
        vTaskDelay(200);
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
            float targetPosition = -calibration.measuredStrokeSteps;
            if (strokeCount % 2 == 0) {
                speed = speed * currentSettings.inSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.maxDepth.getNormalizedModifiedValue(strokeCount);
            } else {
                speed = speed * currentSettings.outSpeed.getNormalizedModifiedValue(strokeCount);
                targetPosition = targetPosition * currentSettings.minDepth.getNormalizedModifiedValue(strokeCount);
            }
            stepper->setSpeedInHz(speed);
            stepper->applySpeedAcceleration();

            float distance = abs(targetPosition - stepper->getCurrentPosition());
            float minAccel = speed / (distance / speed);
            float maxAccel = min(minAccel * 5,Config::Driver::maxAcceleration * (1_mm));
            float accelDifference = maxAccel - minAccel;

            uint32_t acceleration = minAccel;
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
    lastControl = AdvancedControls::SPEED;
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

void incrementControlAdvanced() {
    if(stateMachine->is("advancedPenetration.modifier"_s)) {
        currentSettings.selectedModifierControl = static_cast<AdvancedControls>(
                                                (currentSettings.selectedModifierControl - (int)AdvancedControls::AMPLITUDE + 1) 
                                                % ((int)AdvancedControls::Count2-(int)AdvancedControls::Count - 1)
                                                + (int)AdvancedControls::AMPLITUDE);
    } else {
        currentSettings.selectedControl = static_cast<AdvancedControls>((currentSettings.selectedControl + 1) % (int)AdvancedControls::Count);
    }
}

void setAdvancedChanged() { 
    lastControl = AdvancedControls::SPEED;
}

}