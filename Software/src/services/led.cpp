#include "led.h"

#include <esp_log.h>

#include "components/HeaderBar.h"

CRGB leds[NUM_LEDS];
static auto TAG = "LED";

// BLE status indication functions
static BleStatus lastBLEStatus = BleStatus::DISCONNECTED;
static unsigned long bleStatusChangeTime = 0;
static unsigned long rainbowStartTime = 0;
static int breatheValue = 0;
static bool rainbowActive = false;
static bool dimmed = false;
static bool commPulseActive = false;
static unsigned long advertisingStartTime = 0;  // When advertising started
static const unsigned long ADVERTISING_TIMEOUT = 30000;  // 30 seconds in milliseconds

void initLED() {
    ESP_LOGI(TAG, "Initializing RGB LED on pin %d...", Pins::Display::ledPin);

    FastLED.addLeds<LED_TYPE, Pins::Display::ledPin, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(128);  // Set to 50% brightness by default

    // Turn off LED initially
    setLEDOff();

    ESP_LOGI(TAG, "RGB LED initialization complete.");
}

void setLEDColor(CRGB color) {
    leds[0] = color;
    FastLED.show();
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    leds[0] = CRGB(r, g, b);
    FastLED.show();
}

void setLEDOff() {
    leds[0] = COLOR_OFF;
    FastLED.show();
}

void setLEDBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void flashLED(CRGB color, int duration_ms) {
    setLEDColor(color);
    delay(duration_ms);
    setLEDOff();
}

void breatheLED(CRGB color, int step = 5, int max = 255, int min = 0) {
    static unsigned long lastUpdate = 0;
    static bool increasing = true;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > 10) {
        lastUpdate = currentTime;
        if (increasing) {
            breatheValue += step;
            if (breatheValue >= max) {
                increasing = false;
            }
        } else {
            breatheValue -= step;
            if (breatheValue <= min) {
                increasing = true;
            }
        }

        CRGB dimmedColor = color;
        if (commPulseActive) {
            commPulseActive = false;
            dimmedColor.nscale8(255);
        } else {
            dimmedColor.nscale8(constrain(breatheValue, 0, 255));
        }

        setLEDColor(dimmedColor);
    }
}

void updateLEDForBLEStatus() {
    BleStatus currentStatus = getBleStatus();
    unsigned long currentTime = millis();

    // Check if BLE status changed
    if (currentStatus != lastBLEStatus) {
        lastBLEStatus = currentStatus;
        bleStatusChangeTime = currentTime;
        rainbowActive = false;
        dimmed = false;

        ESP_LOGI(TAG, "BLE status changed to: %d", (int)currentStatus);

        switch (currentStatus) {
            case BleStatus::ADVERTISING:
                // Rainbow will be handled in the continuous loop
                rainbowActive = true;
                rainbowStartTime = currentTime;
                advertisingStartTime = currentTime; // Track when advertising started
                dimmed = false;  // Reset dimmed flag
                break;
            case BleStatus::CONNECTED:
                // Rainbow will be handled in the continuous loop
                rainbowActive = true;
                rainbowStartTime = currentTime;
                dimmed = false;  // Reset advertising flags
                break;
            case BleStatus::DISCONNECTED:
                dimmed = false;  // Reset advertising flags
                break;
            default:
                break;
        }
    }

    // Handle ongoing effects based on current status
    switch (currentStatus) {
        case BleStatus::ADVERTISING:
            if (rainbowActive) {
                showBLERainbow(1000);
                if (millis() - rainbowStartTime >= 1000) {
                    rainbowActive = false;
                }
            } else {
                // Check if advertising timeout has been reached
                unsigned long advertisingElapsed = currentTime - advertisingStartTime;
                if (advertisingElapsed >= ADVERTISING_TIMEOUT && !dimmed) {
                    dimmed = true;
                    ESP_LOGI(TAG,"Advertising timeout reached, dimming LED");
                }

                if (dimmed) {
                    breatheLED(COLOR_BLUE, 1, 50);
                } else {
                    breatheLED(COLOR_BLUE, 5);
                }
            }
            break;

        case BleStatus::CONNECTED:
            if (rainbowActive) {
                showBLERainbow(1000);
                if (millis() - rainbowStartTime >= 1000) {
                    rainbowActive = false;
                }
            } else {
                breatheLED(COLOR_GREEN, 1, 50);
            }
            break;

        default:
            breatheLED(COLOR_CYAN, 1, 50);
            break;
    }
}

void showBLERainbow(int duration_ms) {
    if (!rainbowActive) {
        rainbowActive = true;
        rainbowStartTime = millis();
    }

    unsigned long elapsed = millis() - rainbowStartTime;
    if (elapsed < duration_ms && rainbowActive) {
        // Calculate hue based on time elapsed - full rainbow cycle in
        // duration_ms
        uint8_t hue = (elapsed * 255) / duration_ms;
        setLEDColor(CHSV(hue, 255, 255));
    }
}

// Communication pulse functions
void pulseForCommunication() {
    commPulseActive = true;
}

// Machine status functions
static bool homingActiveFlag = false;

void setHomingActive(bool active) {
    if (homingActiveFlag != active) {
        homingActiveFlag = active;
        ESP_LOGI(TAG, "Homing status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

static bool updateActiveFlag = false;

void setUpdateActive(bool active) {
    if (updateActiveFlag != active) {
        updateActiveFlag = active;
        ESP_LOGI(TAG, "Update status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

static bool wifiSetupActiveFlag = false;

void setWifiSetupActive(bool active) {
    if (wifiSetupActiveFlag != active) {
        wifiSetupActiveFlag = active;
        ESP_LOGI(TAG, "Wifi status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

static bool errorActiveFlag = false;

void setErrorActive(bool active) {
    if (errorActiveFlag != active) {
        errorActiveFlag = active;
        ESP_LOGI(TAG, "Error status changed: %s",
                 active ? "ACTIVE" : "INACTIVE");
    }
}

void updateLEDForMachineStatus() {
    // Check if homing is active - this takes priority over BLE status
    if (errorActiveFlag) {
        breatheLED(COLOR_RED, 51);
    } else if (homingActiveFlag) {
        breatheLED(COLOR_DEEP_PURPLE, 5);
    } else if (wifiSetupActiveFlag) {
        breatheLED(COLOR_YELLOW, 5);
    } else if (updateActiveFlag) {
        breatheLED(COLOR_ORANGE, 51);
    } else {
        // Fall back to BLE status
        updateLEDForBLEStatus();
    }
}