#ifndef OSSM_GPIO_HPP
#define OSSM_GPIO_HPP

#include <regex>

#include "constants/Pins.h"

// Simple helper to map 1-4 to actual board pins
static int mapGpioIndexToPin(int index) {
    switch (index) {
        case 1:
            return Pins::GPIO::pin1;
        case 2:
            return Pins::GPIO::pin2;
        case 3:
            return Pins::GPIO::pin3;
        case 4:
            return Pins::GPIO::pin4;
        default:
            return -1;
    }
}

class GPIOCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& /*connInfo*/) override {
        std::string raw = pCharacteristic->getValue();
        String input = String(raw.c_str());
        input.trim();

        static const std::regex rx(R"(^\s*(\d+)\s*:(low|high|0|1)\s*$)",
                                   std::regex::icase);

        if (!std::regex_match(raw, rx)) {
            static const char err[] PROGMEM = "error:invalid_format";
            ESP_LOGW("NIMBLE", "GPIO write invalid format: %s", raw.c_str());
            pCharacteristic->setValue(String(FPSTR(err)));
            return;
        }

        int sep = input.indexOf(':');
        int index = input.substring(0, sep).toInt();
        String stateStr = input.substring(sep + 1);
        stateStr.toLowerCase();

        int targetPin = mapGpioIndexToPin(index);
        if (targetPin < 0) {
            static const char err[] PROGMEM = "error:pin_out_of_range";
            ESP_LOGW("NIMBLE", "GPIO write out of range index: %d", index);
            pCharacteristic->setValue(String(FPSTR(err)));
            return;
        }

        int level = (stateStr == "high" || stateStr == "1") ? HIGH : LOW;
        digitalWrite(targetPin, level);

        // ESP_LOGD("NIMBLE", "GPIO set pin%d (GPIO %d) to %s", index,
        // targetPin,
        //          level == HIGH ? "HIGH" : "LOW");

        // Respond with normalized acknowledgement
        String ack =
            String("ok:") + index + ":" + (level == HIGH ? "high" : "low");
        pCharacteristic->setValue(ack);
    }

    void onRead(NimBLECharacteristic* pCharacteristic,
                NimBLEConnInfo& /*connInfo*/) override {
        // Return simple capabilities hint
        static const char caps[] PROGMEM = "pins:[1,2,3,4]";
        pCharacteristic->setValue(String(FPSTR(caps)));
    }
} inline gpioCallbacks;


#endif  // OSSM_GPIO_HPP
