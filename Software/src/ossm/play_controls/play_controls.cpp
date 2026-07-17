#include "play_controls.h"

#include "services/board.h"
#include "ossm/state/ble.h"
#include "ossm/state/settings.h"
#include "ossm/state/state.h"
#include "services/encoder.h"
#include "services/tasks.h"
#include "ui.h"
#include "utils/analog.h"
#include "components/HeaderBar.h"

namespace sml = boost::sml;
using namespace sml;

namespace play_controls {

static void drawPlayControlsTask(void *pvParameters) {
    encoder.setAcceleration(10);
    encoder.setBoundaries(0, 100, false);

    switch (settings.playControl) {
        case ui::PlayControls::MIN_POSITION:
            encoder.setEncoderValue(settings.minPosition);
            break;
        case ui::PlayControls::SENSATION:
            encoder.setEncoderValue(settings.sensation);
            break;
        case ui::PlayControls::MAX_POSITION:
            encoder.setEncoderValue(settings.maxPosition);
            break;
        case ui::PlayControls::BUFFER:
            encoder.setEncoderValue(settings.buffer);
            break;
    }

    SettingPercents next = {0, 0, 0, 0, 0, 0, 0, StrokePatterns::SimpleStroke};
    unsigned long displayLastUpdated = 0;

    auto isInCorrectState = []() {
        return stateMachine->is("strokeEngine"_s) ||
               stateMachine->is("strokeEngine.idle"_s) ||
               stateMachine->is("streaming"_s) ||
               stateMachine->is("streaming.idle"_s);
    };

    static float encoderValue = 0;

    bool isStrokeEngine = stateMachine->is("strokeEngine"_s) ||
                          stateMachine->is("strokeEngine.idle"_s);

    bool isStreaming = stateMachine->is("streaming"_s) ||
                       stateMachine->is("streaming.idle"_s);

    bool shouldUpdateDisplay = false;

    showHeaderIcons = true;
    vTaskDelay(100);

    while (isInCorrectState()) {
        shouldUpdateDisplay = false;

        next.speedKnob =
            getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});

        if (abs(next.speedKnob - settings.speedKnob) > 1.0 &&
            next.speedKnob <= settings.speed ) {
            resetLastSpeedCommandWasFromBLE();
        }

        next.speed = next.speedKnob;
        if (settings.speedBLE > 0.0 || wasLastSpeedCommandFromBLE()) {
            if (USE_SPEED_KNOB_AS_LIMIT) {
                next.speed = next.speedKnob * settings.speedBLE / 100;
            } else if (wasLastSpeedCommandFromBLE()) {
                next.speed = settings.speedBLE;
            }
        }

        if (next.speed != settings.speed) {
            shouldUpdateDisplay = true;
            settings.speed = next.speed;
        }
        if (!isDisplayAvailable()) {
            vTaskDelay(100);
            continue;
        }

        settings.speedKnob = next.speedKnob;
        encoderValue = encoder.readEncoder();

        switch (settings.playControl) {
            case ui::PlayControls::MIN_POSITION:
                next.minPosition = encoderValue;
                if (next.minPosition >= settings.maxPosition) {
                    next.minPosition = settings.maxPosition - 1;
                    encoder.setEncoderValue(next.minPosition);
                }
                shouldUpdateDisplay =
                    shouldUpdateDisplay || next.minPosition - settings.minPosition >= 1;
                settings.minPosition = next.minPosition;
                break;
            case ui::PlayControls::SENSATION:
                next.sensation = encoderValue;
                shouldUpdateDisplay =
                    shouldUpdateDisplay || next.sensation - settings.sensation >= 1;
                settings.sensation = next.sensation;
                break;
            case ui::PlayControls::MAX_POSITION:
                next.maxPosition = encoderValue;
                if (next.maxPosition <= settings.minPosition) {
                    next.maxPosition = settings.minPosition + 1;
                    encoder.setEncoderValue(next.maxPosition);
                }
                shouldUpdateDisplay =
                    shouldUpdateDisplay || next.maxPosition - settings.maxPosition >= 1;
                settings.maxPosition = next.maxPosition;
                break;
            case ui::PlayControls::BUFFER:
                next.buffer = encoderValue;
                shouldUpdateDisplay =
                    shouldUpdateDisplay || next.buffer - settings.buffer >= 1;
                settings.buffer = next.buffer;
                break;
        }

        shouldUpdateDisplay =
            shouldUpdateDisplay || millis() - displayLastUpdated > 1000;

        if (!shouldUpdateDisplay) {
            vTaskDelay(100);
            continue;
        }

        displayLastUpdated = millis();

        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            const char *headerText = ui::strings::streaming;
            if (isStrokeEngine) {
                headerText = ui::strings::strokeEngineNames[(int)settings.pattern];
            }

            static String distStr;
            static String timeStr;

            ui::PlayControlsData data{};
            data.speed = next.speed;
            data.minPosition = settings.minPosition;
            data.sensation = settings.sensation;
            data.maxPosition = settings.maxPosition;
            data.buffer = settings.buffer;
            data.activeControl = settings.playControl;
            data.pattern = (int)settings.pattern;
            data.isStrokeEngine = isStrokeEngine;
            data.isStreaming = isStreaming;
            data.headerText = headerText;
            data.speedLabel = ui::strings::speed;
            data.minLabel = ui::strings::min;

            ui::drawPlayControls(display.getU8g2(), data);
            refreshPage(true, true);
            xSemaphoreGive(displayMutex);
        }

        vTaskDelay(200);
    }

    vTaskDelete(nullptr);
};

void drawPlayControls() {
    int stackSize = 3 * configMINIMAL_STACK_SIZE;
    xTaskCreate(drawPlayControlsTask, "drawPlayControlsTask", stackSize,
                nullptr, 1, &Tasks::drawPlayControlsTaskH);
}

}  // namespace play_controls
