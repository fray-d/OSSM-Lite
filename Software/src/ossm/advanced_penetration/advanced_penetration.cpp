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
    ACCELERATION,
    Count
};

struct AdvancedSettings {
    float speed;
    float maxDepth = 10;
    float minDepth = 0;
    float acceleration = 50;
    float speedKnob;
    AdvancedControls control = AdvancedControls::MAX_DEPTH;
    bool operator==(const AdvancedSettings &a) const {
        return this->speed == a.speed
        && this->maxDepth == a.maxDepth
        && this->minDepth == a.minDepth
        && this->acceleration == a.acceleration
        && abs(this->speedKnob - a.speedKnob) <= 1;
    }
    bool operator!=(const AdvancedSettings &a) const {
        return !(*this == a);
    }
};

AdvancedSettings currentSettings = {0,0};

static void startAdvancedPenetrationTask(void *pvParameters) {
    auto isInCorrectState = []() {
        // Add any states that you want to support here.
        return stateMachine->is("advancedPenetration"_s) ||
               stateMachine->is("advancedPenetration.idle"_s);
    };

    AdvancedSettings lastSettings = {1,1};
    static float encoderValue = 0;
    encoder.setAcceleration(10);
    encoder.setBoundaries(0, 100, false);
    setControlEncoder();

    while (isInCorrectState()) {
        currentSettings.speedKnob = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        encoderValue = encoder.readEncoder();

        switch(currentSettings.control) {
            case AdvancedControls::MAX_DEPTH:
                if (encoderValue >= currentSettings.minDepth){
                    currentSettings.maxDepth = encoderValue;
                } else {
                    encoder.setEncoderValue(currentSettings.minDepth);
                }
                break;
            case AdvancedControls::MIN_DEPTH:
                if (encoderValue <= currentSettings.maxDepth) {
                    currentSettings.minDepth = encoderValue;
                } else {
                    encoder.setEncoderValue(currentSettings.maxDepth);
                }
                break;
            case AdvancedControls::ACCELERATION:
                currentSettings.acceleration = encoderValue;
                break;
            default:
                break;
        }

        if (currentSettings != lastSettings) {
            currentSettings.speed = currentSettings.speedKnob;
            lastSettings = currentSettings;
            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                String headerText = UserConfig::language.AdvancedPenetration;
                setHeader(headerText);
                clearPage(true);

                drawShape::settingBar("", currentSettings.speed);
                display.setFont(Config::Font::bold);
                int controlPosition = 125;
                String controlText = "";

                if (currentSettings.control == AdvancedControls::ACCELERATION) {
                    controlText = String("Acceleration");
                    controlPosition += 3;
                    drawShape::settingBar("",currentSettings.acceleration, controlPosition, 10, RIGHT_ALIGNED, 0, 40);
                    controlPosition -= 15;
                } else {
                    drawShape::settingBarSmall(currentSettings.acceleration, controlPosition, 10, 40);
                    controlPosition -= 5;
                }
                if (currentSettings.control == AdvancedControls::MIN_DEPTH) {
                    controlText = String("Min Depth");
                    controlPosition += 3;
                    drawShape::settingBar("",currentSettings.minDepth, controlPosition, 10, RIGHT_ALIGNED, 0, 40);
                    controlPosition -= 15;
                } else {
                    drawShape::settingBarSmall(currentSettings.minDepth, controlPosition, 10, 40);
                    controlPosition -= 5;
                }
                if (currentSettings.control == AdvancedControls::MAX_DEPTH) {
                    controlText = String("Max Depth");
                    controlPosition += 3;
                    drawShape::settingBar("",currentSettings.maxDepth, controlPosition, 10, RIGHT_ALIGNED, 0, 40);
                    controlPosition -= 15;
                } else {
                    drawShape::settingBarSmall(currentSettings.maxDepth, controlPosition, 10, 40);
                    controlPosition -= 5;
                }
                drawShape::settingBarSmall(currentSettings.minDepth, controlPosition, 10, 40);
                int stringWidth = display.getUTF8Width(controlText.c_str());
                display.drawUTF8(128 - stringWidth, 64, controlText.c_str());

                refreshPage(true);
                xSemaphoreGive(displayMutex);
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
}

void incrementControlAdvanced() {
    currentSettings.control = static_cast<AdvancedControls>((currentSettings.control + 1) % (int)AdvancedControls::Count);
    setControlEncoder();
}

void setControlEncoder() {
    switch (currentSettings.control) {
        case AdvancedControls::MAX_DEPTH:
            encoder.setEncoderValue(currentSettings.maxDepth);
            break;
        case AdvancedControls::MIN_DEPTH:
            encoder.setEncoderValue(currentSettings.minDepth);
            break;
        case AdvancedControls::ACCELERATION:
            encoder.setEncoderValue(currentSettings.acceleration);
            break;
        default:
            break;
    }
}

}