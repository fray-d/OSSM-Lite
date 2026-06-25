#include "pattern_controls.h"

#include "Strings.h"
#include "constants/Pins.h"
#include "services/board.h"
#include "ossm/state/settings.h"
#include "ossm/state/state.h"
#include "services/display.h"
#include "services/encoder.h"
#include "services/tasks.h"
#include "ui.h"
#include "utils/analog.h"
#include "components/HeaderBar.h"

namespace sml = boost::sml;
using namespace sml;

namespace pattern_controls {

static size_t numberOfDescriptions = sizeof(ui::strings::strokeEngineDescriptions);
static size_t numberOfPatterns = sizeof(ui::strings::strokeEngineNames);

static void drawPatternControlsTask(void *pvParameters) {
    auto isInCorrectState = []() {
        return stateMachine->is("strokeEngine.pattern"_s);
    };

    StrokePatterns nextPattern = settings.pattern;
    bool shouldUpdateDisplay = true;

    encoder.setAcceleration(10);
    encoder.setBoundaries(0, numberOfPatterns * 3 - 1, true);
    encoder.setEncoderValue((int)nextPattern * 3);

    showHeaderIcons = true;

    while (isInCorrectState()) {
        settings.speedKnob = getAnalogAveragePercent(SampleOnPin{Pins::Remote::speedPotPin, 50});
        if (settings.speedKnob != settings.speed) {
            shouldUpdateDisplay = true;
            settings.speed = settings.speedKnob;
        }

        nextPattern = StrokePatterns(encoder.readEncoder() / 3);
        shouldUpdateDisplay = shouldUpdateDisplay || settings.pattern != nextPattern;
        if (!shouldUpdateDisplay) {
            vTaskDelay(100);
            continue;
        }

        const char *patternName = ui::strings::strokeEngineNames[(int)nextPattern];
        const char *patternDescription = ui::strings::noDescription;

        if ((int)nextPattern < (int)numberOfDescriptions) {
            patternDescription = ui::strings::strokeEngineDescriptions[(int)nextPattern];
        }

        settings.pattern = nextPattern;

        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            ui::TextPage page;
            page.title = patternName;
            page.body = patternDescription;
            page.scrollPercent = ui::scrollPercent((int)nextPattern, numberOfPatterns);
            ui::drawTextPage(display.getU8g2(), page);
            refreshPage(true, true);
            xSemaphoreGive(displayMutex);
        }

        vTaskDelay(200);
    }

    vTaskDelete(nullptr);
};

void drawPatternControls() {
    int stackSize = 3 * configMINIMAL_STACK_SIZE;
    xTaskCreate(drawPatternControlsTask, "drawPatternControlsTask", stackSize,
                nullptr, 1, &Tasks::drawPatternControlsTaskH);
}

}  // namespace pattern_controls