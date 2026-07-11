#include "homing.h"

#include "Strings.h"
#include "components/HeaderBar.h"
#include "constants/Pins.h"
#include "constants/Version.h"
#include "HelloAnimation.h"
#include "ossm/Events.h"
#include "ossm/state/calibration.h"
#include "ossm/state/error.h"
#include "ossm/state/state.h"
#include "services/display.h"
#include "services/led.h"
#include "services/stepper.h"
#include "services/tasks.h"
#include "services/UserConfig.h"
#include "Strings.h"
#include "utils/analog.h"
#include "ui.h"

namespace sml = boost::sml;
using namespace sml;

namespace homing {
    void clearHoming() {
        ESP_LOGD("Homing", "Homing started");

        // Set acceleration and deceleration in steps/s^2
        stepper->setAcceleration(UserConfig::getStepsPerMM(1000));
        // Set speed in steps/s
        stepper->setSpeedInHz(UserConfig::getStepsPerMM(UserConfig::getHomingSpeed()));

        // Recalibrate the current sensor offset.
        calibration.currentSensorOffset = getAnalogAveragePercent(SampleOnPin{Pins::Driver::currentSensorPin, 1000});
    }

    static void startHomingTask(void *pvParameters) {
        TickType_t xTaskStartTime = xTaskGetTickCount();

        stepper->enableOutputs();
        stepper->setDirectionPin(Pins::Driver::motorDirectionPin, UserConfig::getDirection());

        bool isDouble = UserConfig::getHomingType() == UserConfig::HomingType::DoubleTap;
        bool isNone = UserConfig::getHomingType() == UserConfig::HomingType::None;
        bool isSingle = UserConfig::getHomingType() == UserConfig::HomingType::SingleSided;

        if (isNone && calibration.isFirstHomed) {
            stepper->setCurrentPosition(-1 * UserConfig::getStepsPerMM(10));
        }

        if (isNone || (isSingle && stateMachine->is("measure.run"_s) )) {
            calibration.measuredStrokeSteps = UserConfig::getStepsPerMM(UserConfig::getRailLength());
            stateMachine->process_event(Done{});
            vTaskDelete(nullptr);
            return;
        }

        if (!UserConfig::getReHome() && !calibration.isFirstHomed) {
            stateMachine->process_event(Done{});
            vTaskDelete(nullptr);
            return;
        }

        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            if (stateMachine->is("homing.run"_s)) {
            ui::drawHelloFrame(display.getU8g2(), ui::HELLO_FRAMES[ui::HELLO_FRAME_COUNT - 1], VERSION, ui::strings::homing);
            } else {
            ui::drawHelloFrame(display.getU8g2(), ui::HELLO_FRAMES[ui::HELLO_FRAME_COUNT - 1], VERSION, ui::strings::measuringRail);
            }
            refreshPage(true, true);
            xSemaphoreGive(displayMutex);
        }

        float maxRail = UserConfig::getMaxRailLength();
        int16_t sign = stateMachine->is("homing.run"_s) ? -1 : 1;
        int32_t targetPositionInSteps = round(sign * UserConfig::getStepsPerMM(maxRail));

        ESP_LOGD("Homing", "Target position in steps: %d",
                 targetPositionInSteps);
        stepper->moveTo(targetPositionInSteps, false);

        auto isInCorrectState = []() {
            return stateMachine->is("homing"_s) ||
                   stateMachine->is("measure.run"_s) ||
                   stateMachine->is("homing.run"_s);
        };

        bool second = true;
        if (isDouble) {
            second = false;
        }
        while (isInCorrectState()) {
            TickType_t xCurrentTickCount = xTaskGetTickCount();
            TickType_t xTicksPassed = xCurrentTickCount - xTaskStartTime;
            uint32_t msPassed = xTicksPassed * portTICK_PERIOD_MS;
            if (msPassed > maxRail / (UserConfig::getHomingSpeed() / 1000.0)) {
                ESP_LOGE("Homing", "Homing took too long. Check power and restart");
                errorState.message = ui::strings::homingTookTooLong;

                stateMachine->process_event(Error{});
                break;
            }

            float current = getAnalogAveragePercent(SampleOnPin{
                                Pins::Driver::currentSensorPin, 50}) -
                            calibration.currentSensorOffset;

            ESP_LOGV("Homing", "Current: %f", current);

            if (current < UserConfig::getSensorLimit()) {
                vTaskDelay(10);  // 10ms to reduce CPU load
                continue;
            }

            ESP_LOGD("Homing", "Current over limit: %f", current);
            stepper->stopMove();

            stepper->setSpeedInHz(UserConfig::getStepsPerMM(UserConfig::getHomingSpeed() * 10.0));
            int32_t currentPosition = stepper->getCurrentPosition();
            stepper->moveTo(currentPosition - sign * UserConfig::getStepsPerMM(10), true);

            // measure and save the current position
            calibration.measuredStrokeSteps = std::max(std::abs(float(stepper->getCurrentPosition())),calibration.measuredStrokeSteps);
            calibration.measuredStrokeSteps = std::min(calibration.measuredStrokeSteps,UserConfig::getStepsPerMM(maxRail));

            if (!second && stateMachine->is("homing.run"_s) &&
                        abs(stepper->getCurrentPosition()) < UserConfig::getStepsPerMM(UserConfig::getMinRailLength())) {
                second = true;
                stepper->setSpeedInHz(UserConfig::getStepsPerMM(UserConfig::getHomingSpeed()));
                stepper->moveTo(targetPositionInSteps, false);
                continue;
            }

            ESP_LOGI("Homing", "Steps: %f", calibration.measuredStrokeSteps);

            if (stateMachine->is("homing.run"_s)) {
                stepper->forceStopAndNewPosition(0);
            }

            stepper->setSpeedInHz(UserConfig::getStepsPerMM(UserConfig::getHomingSpeed() * 10));
            stepper->moveTo(0, true);
            if (!isSingle && stateMachine->is("homing.run"_s) && calibration.isFirstHomed) {
                stepper->moveTo(calibration.measuredStrokeSteps, true);
            }

            stateMachine->process_event(Done{});
            break;
        };
        
        vTaskDelete(nullptr);
    }

    void startHoming() {
        int stackSize = 10 * configMINIMAL_STACK_SIZE;
        xTaskCreatePinnedToCore(startHomingTask, "startHomingTask", stackSize,
                                nullptr, configMAX_PRIORITIES - 1,
                                &Tasks::runHomingTaskH,
                                Tasks::operationTaskCore);
    }

    bool isStrokeTooShort() {
        if (calibration.measuredStrokeSteps >=
            UserConfig::getStepsPerMM(UserConfig::getMinRailLength())) {
            return false;
        }
        errorState.message = ui::strings::strokeTooShort;
        return true;
    }

}  // namespace homing
