#include "advanced_penetration.h"

#include "constants/Config.h"
#include "utils/analog.h"
#include "services/display.h"
#include "extensions/u8g2Extensions.h"
#include "ossm/state/calibration.h"
#include "ossm/state/state.h"
#include "services/encoder.h"
#include "services/stepper.h"
#include "services/tasks.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {

enum AdvancedControls {
    MAX_DEPTH,
    MIN_DEPTH,
    IN_SPEED,
    OUT_SPEED,
    ACCELERATION,
    Count,
    SPEED //uncounted
};

struct AdvancedControl {
    AdvancedControls id;
    float value;
    String name;
};

struct AdvancedSettings {
    AdvancedControl speed = {AdvancedControls::SPEED,0,"Speed"};
    AdvancedControl inSpeed = {AdvancedControls::IN_SPEED,100,"In Speed"};
    AdvancedControl outSpeed = {AdvancedControls::OUT_SPEED,100,"Out Speed"};
    AdvancedControl maxDepth = {AdvancedControls::MAX_DEPTH,10,"Max Depth"};
    AdvancedControl minDepth = {AdvancedControls::MIN_DEPTH,0,"Min Depth"};
    AdvancedControl acceleration = {AdvancedControls::ACCELERATION, 50, "Acceleration"};

    AdvancedControls selectedControl = AdvancedControls::MAX_DEPTH;
    bool changed = false;
};

AdvancedSettings currentSettings;
uint8_t controlPosition;
AdvancedControls lastControl = AdvancedControls::SPEED;

void updateControl(AdvancedControl& a, float minVal = 0, float maxVal = 100) {
    if (currentSettings.selectedControl == a.id) {
        if (lastControl != a.id) {
            encoder.setBoundaries(minVal, maxVal, false);
            encoder.setEncoderValue(a.value);
            lastControl = a.id;
        } 
        a.value = constrain(encoder.readEncoder(),minVal, maxVal);
        String controlText = a.name;
        uint16_t stringWidth = display.getUTF8Width(controlText.c_str());
        display.drawUTF8(128 - stringWidth, 64, controlText.c_str());
        controlPosition += 3;
        drawShape::settingBar("",a.value, controlPosition, 10, RIGHT_ALIGNED, 0, 40);
        controlPosition -= 15;
    } else {
        drawShape::settingBarSmall(a.value, controlPosition, 10, 40);
        controlPosition -= 5;
    }
}

bool isInCorrectState() {
    return stateMachine->is("advancedPenetration"_s)
        || stateMachine->is("advancedPenetration.idle"_s);
};

static void startAdvancedPenetrationTask(void *pvParameters) {

    encoder.setAcceleration(10);
    float lastEncoder = 0;

    while (isInCorrectState()) {
        float speedValue = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        float encoderValue = encoder.readEncoder();
        if ( abs(speedValue - currentSettings.speed.value) > 1
                    || encoderValue != lastEncoder 
                    || currentSettings.selectedControl != lastControl) {
            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                currentSettings.speed.value = speedValue;
                lastEncoder = encoderValue;

                String headerText = UserConfig::language.AdvancedPenetration;
                setHeader(headerText);
                clearPage(true);

                drawShape::settingBar("", currentSettings.speed.value);
                controlPosition = 125;

                //Reverse order... right to left.
                updateControl(currentSettings.acceleration, 1);
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

float rampValue(float value, float exp){
    return pow( 1 - pow( 1 - value, exp), 1 / exp);
}

static void startAdvancedPenetrationMotionTask(void *pvParameters) {
    int strokeCount = 0;
    while (isInCorrectState()) {
        if (currentSettings.speed.value == 0.0) {
            stepper->stopMove();
            continue;
        }
        if (stepper->isRunning()) {
            vTaskDelay(1);
        } else {
            strokeCount ++;
        }
        if (currentSettings.changed || !stepper->isRunning()) {
            currentSettings.changed = false;
            float speed = Config::Driver::maxSpeedMmPerSecond * (1_mm) * rampValue(currentSettings.speed.value / 100.0, 0.8);
            float targetPosition = -calibration.measuredStrokeSteps;
            if (strokeCount % 2 == 0) {
                speed = speed * currentSettings.inSpeed.value / 100.0;
                targetPosition = targetPosition * currentSettings.maxDepth.value / 100.0;
            } else {
                speed = speed * currentSettings.outSpeed.value / 100.0;
                targetPosition = targetPosition * currentSettings.minDepth.value / 100.0;
            }
            stepper->setSpeedInHz(speed);
            stepper->applySpeedAcceleration();

            float distance = abs(targetPosition - stepper->getCurrentPosition());
            float minAccel = speed / (distance / speed);
            float maxAccel = min(minAccel * 5,Config::Driver::maxAcceleration * (1_mm));
            float accelDifference = maxAccel - minAccel;

            uint32_t acceleration = accelDifference * rampValue(currentSettings.acceleration.value / 100.0, 0.8) + minAccel;
            float oldAcceleration = stepper->getAcceleration();
            if (acceleration > oldAcceleration || !stepper->isRunning()){
                stepper->setAcceleration(acceleration);
                stepper->applySpeedAcceleration();
            }
            if (!stepper->isRunning()) {
                stepper->moveTo(targetPosition,false);
            }
        }
    }
    vTaskDelete(nullptr);
}

void startAdvancedPenetration() {
    int stackSize = 10 * configMINIMAL_STACK_SIZE;

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
    currentSettings.selectedControl = static_cast<AdvancedControls>((currentSettings.selectedControl + 1) % (int)AdvancedControls::Count);
}

}