#include "advanced_penetration.h"

#include "constants/Config.h"
#include "services/display.h"
#include "extensions/u8g2Extensions.h"
#include "ossm/state/calibration.h"
#include "ossm/state/state.h"
#include "services/stepper.h"
#include "services/tasks.h"

namespace sml = boost::sml;
using namespace sml;

namespace advanced_penetration {


static void startAdvancedPenetrationTask(void *pvParameters) {
    auto isInCorrectState = []() {
        // Add any states that you want to support here.
        return stateMachine->is("advancedPenetration"_s) ||
               stateMachine->is("advancedPenetration.idle"_s);
    };

    while (isInCorrectState()) {


        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            // Check and update the header text... don't worry if this is the
            // same as last time, nothing happens.
            String headerText = UserConfig::language.AdvancedPenetration;
            setHeader(headerText);

            // Now draw the page...
            clearPage(true);
            display.setFont(Config::Font::base);

            drawShape::settingBar(UserConfig::language.Speed, 0);

            

            refreshPage(true);
            xSemaphoreGive(displayMutex);
        }


        vTaskDelay(100);
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

}