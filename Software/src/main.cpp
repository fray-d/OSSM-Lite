#include "Arduino.h"
#include "OneButton.h"
#include "components/HeaderBar.h"
#include "ossm/state/state.h"
#include "services/board.h"
#include "services/communication/nimble.h"
#include "services/display.h"
#include "services/led.h"
#include "services/wm.h"

namespace sml = boost::sml;
using namespace sml;

/*
 *  ██████╗ ███████╗███████╗███╗   ███╗    ██╗     ██╗████████╗███████╗
 * ██╔═══██╗██╔════╝██╔════╝████╗ ████║    ██║     ██║╚══██╔══╝██╔════╝
 * ██║   ██║███████╗███████╗██╔████╔██║    ██║     ██║   ██║   █████╗
 * ██║   ██║╚════██║╚════██║██║╚██╔╝██║    ██║     ██║   ██║   ██╔══╝
 * ╚██████╔╝███████║███████║██║ ╚═╝ ██║    ███████╗██║   ██║   ███████╗
 *  ╚═════╝ ╚══════╝╚══════╝╚═╝     ╚═╝    ╚══════╝╚═╝   ╚═╝   ╚══════╝

 *
 * Welcome to the open source sex machine, lite edition!
 * This is fork of the OSSM and is licensed under the MIT license.
 *
 * The primary goal is to remove some of the heavy integration with R+D
 * systems, simplify the user experience, and clean up unused code.
 */

OneButton button(Pins::Remote::encoderSwitch, false);

void __attribute__((weak)) setup() {
    // Suppress verbose GPIO configuration logs
    esp_log_level_set("gpio", ESP_LOG_WARN);

    /** Board setup */
    initBoard();
    ESP_LOGD("MAIN", "Starting OSSM");

    // Display
    initDisplay();
    ESP_LOGD("Main", "Display %s", isDisplayAvailable() ? "connected":"disconnected");

    // Initialize header bar task
    initHeaderBar();

    // Initialize state machine after global state is set up
    initStateMachine();

    // ialize LED for BLE and machine status indication
    ESP_LOGI("MAIN", "LED initialized for BLE and machine status indication");
    updateLEDForMachineStatus();  // Set initial LED state

    // // link functions to be called on events.
    button.attachClick([]() { stateMachine->process_event(ButtonPress{}); });
    button.attachDoubleClick([]() { stateMachine->process_event(DoublePress{}); });
    button.attachLongPressStart([]() { stateMachine->process_event(LongPress{}); });

    xTaskCreatePinnedToCore(
        [](void *pvParameters) {
            while (true) {
                button.tick();
                vTaskDelay(25 / portTICK_PERIOD_MS);
            }
        },
        "buttonTask", 4 * configMINIMAL_STACK_SIZE, nullptr,
        configMAX_PRIORITIES - 1, nullptr, 0);

    // Initialize NimBLE only when in menu.idle state
    xTaskCreatePinnedToCore(
        [](void *pvParameters) {
            bool initialized = false;
            while (true) {
                if ((stateMachine->is("menu.idle"_s) ||
                     stateMachine->is("error.idle"_s)) &&
                    !initialized) {
                    ESP_LOGD("MAIN", "Initializing communication services");
                    initNimble();
                    initWM();
                    initialized = true;
                    vTaskDelete(nullptr);
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        },
        "initNimbleTask", 32 * configMINIMAL_STACK_SIZE, nullptr,
        configMAX_PRIORITIES - 1, nullptr, 0);
};

void __attribute__((weak)) loop() { vTaskDelete(nullptr); };

