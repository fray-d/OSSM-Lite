#include "hello.h"

#include <string>

#include "HelloAnimation.h"
#include "Logos.h"
#include "Strings.h"
#include "components/HeaderBar.h"
#include "constants/Version.h"
#include "services/display.h"
#include "services/tasks.h"
#include "ui.h"

namespace pages {

static void drawHelloTask(void *pvParameters) {
    showHeaderIcons = false;

    for (int i = 0; i < ui::HELLO_FRAME_COUNT; i++) {
        if (xSemaphoreTake(displayMutex, 100) == pdTRUE) {
            ui::drawHelloFrame(display.getU8g2(), ui::HELLO_FRAMES[i],
                               VERSION);
            refreshPage(true, true);
            xSemaphoreGive(displayMutex);
        }
        vTaskDelay(1);
    }
    vTaskDelete(nullptr);
}

void drawHello() {
    if(!isDisplayAvailable()) return;
    int stackSize = 3 * configMINIMAL_STACK_SIZE;
    xTaskCreate(drawHelloTask, "drawHello", stackSize, nullptr, 1,
                &Tasks::drawHelloTaskH);
}

}  // namespace pages
