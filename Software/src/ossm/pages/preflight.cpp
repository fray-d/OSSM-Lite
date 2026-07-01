#include "preflight.h"

#include "components/HeaderBar.h"
#include "ossm/Events.h"
#include "ossm/state/menu.h"
#include "ossm/state/state.h"
#include "services/tasks.h"
#include "ui.h"
#include "utils/analog.h"

namespace sml = boost::sml;
using namespace sml;

namespace pages {

    static void drawPreflightTask(void *pvParameters) {
        auto menuString = menuStrings[menuState.currentOption];
        float speedPercentage;

        auto isInPreflight = []() {
            return stateMachine->is("advancedPenetration.preflight"_s) ||
                   stateMachine->is("streaming.preflight"_s) ||
                   stateMachine->is("strokeEngine.preflight"_s);
        };

        showHeaderIcons = true;

        do {
            speedPercentage = getAnalogAveragePercent(
                SampleOnPin{Pins::Remote::speedPotPin, 50});
            if (speedPercentage < 1.0) {
                stateMachine->process_event(Done{});
                break;
            };

            if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
                ui::PreflightData data{menuString, speedPercentage,
                                       ui::strings::speed,
                                       ui::strings::speedWarning};
                ui::drawPreflight(display.getU8g2(), data);
                refreshPage(true, true);
                xSemaphoreGive(displayMutex);
            }

            vTaskDelay(100);
        } while (isInPreflight());

        vTaskDelete(nullptr);
    };

    void drawPreflight() {
        if (!isDisplayAvailable()) return;
        int stackSize = 3 * configMINIMAL_STACK_SIZE;
        xTaskCreate(drawPreflightTask, "drawPreflightTask", stackSize, nullptr,
                    1, &Tasks::drawPreflightTaskH);
    }

}  // namespace pages
